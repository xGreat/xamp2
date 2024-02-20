#include <output_device/win32/sharedwasapidevice.h>

#ifdef XAMP_OS_WIN
#include <output_device/iaudiocallback.h>
#include <output_device/win32/unknownimpl.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/mmcss.h>

#include <base/base.h>
#include <base/assert.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/waitabletimer.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

using namespace helper;

/*
* SetWaveformatEx is a helper function to set WAVEFORMATEX.
* 
* @param[in] input_fromat WAVEFORMATEX*
* @param[in] samplerate uint32_t
*/
static void SetWaveformatEx(WAVEFORMATEX *input_fromat, uint32_t samplerate) noexcept {
	XAMP_EXPECTS(input_fromat != nullptr);
	XAMP_EXPECTS(input_fromat->nChannels == AudioFormat::kMaxChannel);
	XAMP_EXPECTS(samplerate > 0);

	auto &format = *reinterpret_cast<WAVEFORMATEXTENSIBLE *>(input_fromat);

	format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	format.Format.nChannels = 2;
	format.Format.nBlockAlign = 2 * sizeof(float);
	format.Format.wBitsPerSample = 8 * sizeof(float);
	format.Format.cbSize = 22;	
	format.Format.nSamplesPerSec = samplerate;
	format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nBlockAlign;
	format.Samples.wValidBitsPerSample = format.Format.wBitsPerSample;
	format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
	format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
}

inline constexpr IID kSimpleAudioVolumeID = __uuidof(ISimpleAudioVolume);
inline constexpr IID kAudioEndpointVolumeCallbackID = __uuidof(IAudioEndpointVolumeCallback);
inline constexpr IID kAudioEndpointVolumeID = __uuidof(IAudioEndpointVolume);
inline constexpr IID kAudioRenderClientID =__uuidof(IAudioRenderClient);
inline constexpr IID kAudioClient3ID = __uuidof(IAudioClient3);
inline constexpr IID kAudioClockID = __uuidof(IAudioClock);

/*
* DeviceEventNotification is a IAudioEndpointVolumeCallback implementation.
*/
class SharedWasapiDevice::DeviceEventNotification final
	: public UnknownImpl<IAudioEndpointVolumeCallback> {
public:
	/*
	* Constructor
	*/
	explicit DeviceEventNotification(IAudioCallback* callback) noexcept
		: callback_(callback) {
		XAMP_EXPECTS(callback_ != nullptr);
	}

	/*
	* Destructor
	*/
	virtual ~DeviceEventNotification() override = default;
	
	/*
	* QueryInterface
	* 
	* @param[in] iid REFIID
	* @param[out] ReturnValue void**
	* 
	* @return HRESULT
	*/
	HRESULT QueryInterface(REFIID iid, void** ReturnValue) override {		
		if (ReturnValue == nullptr) {
			return E_POINTER;
		}
		*ReturnValue = nullptr;
		if (iid == IID_IUnknown) {
			*ReturnValue = static_cast<IUnknown*>(static_cast<IAudioEndpointVolumeCallback*>(this));
			AddRef();
		}
		else if (iid == kAudioEndpointVolumeCallbackID) {
			*ReturnValue = static_cast<IAudioEndpointVolumeCallback*>(this);
			AddRef();
		}
		else {
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	/*
	* OnNotify
	* 
	* @param[in] NotificationData PAUDIO_VOLUME_NOTIFICATION_DATA	
	*/
	STDMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA NotificationData) override {	
		callback_->OnVolumeChange(static_cast<int32_t>(NotificationData->fMasterVolume * 100.0f));
		return S_OK;
	}

private:
	IAudioCallback* callback_;
};

SharedWasapiDevice::SharedWasapiDevice(bool is_low_latency, CComPtr<IMMDevice> const & device)
	: is_low_latency_(is_low_latency)
	, is_running_(false)
	, stream_time_(0)
	, buffer_frames_(0)
	, buffer_time_(0)
	, mmcss_name_(kMmcssProfileProAudio)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_HIGH)
	, sample_ready_(nullptr)
	, device_(device)
	, callback_(nullptr)
	, logger_(LoggerManager::GetInstance().GetLogger(kSharedWasapiDeviceLoggerName)) {
}

SharedWasapiDevice::~SharedWasapiDevice() {
    try {
        CloseStream();
        sample_ready_.reset();
    } catch (...) {
    }
	UnRegisterDeviceVolumeChange();
}

void SharedWasapiDevice::UnRegisterDeviceVolumeChange() {
	if (endpoint_volume_ != nullptr) {
		endpoint_volume_->UnregisterControlChangeNotify(device_volume_notification_);
	}
}

void SharedWasapiDevice::RegisterDeviceVolumeChange() {
	HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume_)
	));
	device_volume_notification_ = new DeviceEventNotification(callback_);
	HrIfFailledThrow(endpoint_volume_->RegisterControlChangeNotify(device_volume_notification_));
}

bool SharedWasapiDevice::IsStreamOpen() const noexcept {
	return render_client_ != nullptr;
}

void SharedWasapiDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
	callback_ = callback;
}

void SharedWasapiDevice::StopStream(bool wait_for_stop_stream) {
	XAMP_LOG_D(logger_, "StopStream is_running_: {}", is_running_);
	if (!is_running_) {
		return;
	}

	is_running_ = false;
	rt_work_queue_->Destroy();
	client_->Stop();
}

void SharedWasapiDevice::CloseStream() {
	XAMP_LOG_D(logger_, "CloseStream is_running_: {}", is_running_);

	UnRegisterDeviceVolumeChange();
	endpoint_volume_.Release();
	simple_audio_volume_.Release();
	device_volume_notification_.Release();

	// We don't close sample ready event immediately,
	// WASAPI can be in same sample rate payback to reset and reuse sample ready event.
	//sample_ready_.close();
	render_client_.Release();
	clock_.Release();
	rt_work_queue_.Release();
}

void SharedWasapiDevice::InitialDeviceFormat(const AudioFormat& output_format) {
	uint32_t fundamental_period_in_frame = 0;
	uint32_t current_period_in_frame = 0;
	uint32_t default_period_in_frame = 0;
	uint32_t max_period_in_frame = 0;
	uint32_t min_period_in_frame = 0;

	mix_format_.Free();

	// Get the mix format and the current shared mode engine period.
	HrIfFailledThrow(client_->GetCurrentSharedModeEnginePeriod(&mix_format_, &current_period_in_frame));

	// Set the mix format to the device format.
	SetWaveformatEx(mix_format_, output_format.GetSampleRate());

	// The pFormat parameter below is optional (Its needed only for MATCH_FORMAT clients).
	const auto hr = client_->GetSharedModeEnginePeriod(mix_format_,
		&default_period_in_frame,
		&fundamental_period_in_frame,
		&min_period_in_frame,
		&max_period_in_frame);

	if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
		throw DeviceUnSupportedFormatException(output_format);
	}

	auto msec_per_samples = 1000.0 / static_cast<double>(output_format.GetSampleRate());

	XAMP_LOG_D(logger_,
		"Initial device format fundamental:{:.2f} msec, current:{:.2f} msec, min:{:.2f} msec max:{:.2f} msec.",
		fundamental_period_in_frame * msec_per_samples,
		default_period_in_frame * msec_per_samples,
		min_period_in_frame * msec_per_samples,
		max_period_in_frame * msec_per_samples);

	buffer_time_ = default_period_in_frame;
	//buffer_time_ = current_period_in_frame;

	XAMP_LOG_D(logger_, "Use latency: {:.2f}", buffer_time_ * msec_per_samples);
}

void SharedWasapiDevice::InitialDevice(const AudioFormat& output_format) {
	if (!is_low_latency_) {
		InitialDeviceFormat(output_format);
		auto hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			buffer_time_,
			buffer_time_,
			mix_format_,
			nullptr);
		if (hr == HRESULT_FROM_WIN32(ERROR_BUSY)) {
			throw DeviceInUseException();
		}
		if (hr == AUDCLNT_E_ENGINE_PERIODICITY_LOCKED) {
			// 會出現這個錯誤, 代表音效設備不支援同時多個 sample rate, 所以需要進行重採樣轉換.			
			throw DeviceNeedSetMatchFormatException();
		}
		HrIfFailledThrow(hr);
	} else {
		InitialDeviceFormat(output_format);
		auto hr = client_->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			buffer_time_,
			mix_format_,
			nullptr);
		if (hr == HRESULT_FROM_WIN32(ERROR_BUSY)) {
			throw DeviceInUseException();
		}
		HrIfFailledThrow(hr);		
	}
}

void SharedWasapiDevice::OpenStream(AudioFormat const & output_format) {
	stream_time_ = 0;

	if (!client_) {
		XAMP_LOG_D(logger_, "Active device format: {}.", output_format);
		
		HrIfFailledThrow(device_->Activate(kAudioClient3ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

		// Try to raw mode in client properties.
		AudioClientProperties device_props{};
		device_props.bIsOffload = FALSE;
		device_props.cbSize = sizeof(device_props);
		device_props.eCategory = AudioCategory_Media;
		device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT | AUDCLNT_STREAMOPTIONS_RAW;
		try {
			HrIfFailledThrow(client_->SetClientProperties(&device_props));
		}
		catch (const Exception& e) {
			XAMP_LOG_D(logger_, "SetClientProperties return failure! {}", e.GetErrorMessage());
			device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
			HrIfFailledThrow(client_->SetClientProperties(&device_props));
		}

		InitialDevice(output_format);
	}

	RegisterDeviceVolumeChange();

	// Reset device state.
	HrIfFailledThrow(client_->Reset());

	// Get the buffer size.
	HrIfFailledThrow(client_->GetBufferSize(&buffer_frames_));

	// Get the render client.
	HrIfFailledThrow(client_->GetService(kAudioRenderClientID, 
		reinterpret_cast<void**>(&render_client_)));

	// Get the audio clock.
	HrIfFailledThrow(client_->GetService(kAudioClockID,
		reinterpret_cast<void**>(&clock_)));

	XAMP_LOG_D(logger_, "WASAPI buffer frame size:{}.", buffer_frames_);

	// Create sample ready event.
	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		XAMP_ASSERT(sample_ready_);
		HrIfFailledThrow(client_->SetEventHandle(sample_ready_.get()));
	}

	// Create the work queue.
	rt_work_queue_ = MakeWasapiWorkQueue(mmcss_name_, this, &SharedWasapiDevice::OnInvoke);

	// Get the device volume interface.
	HrIfFailledThrow(client_->GetService(kSimpleAudioVolumeID, reinterpret_cast<void**>(&simple_audio_volume_)));
}

bool SharedWasapiDevice::IsMuted() const {
	BOOL is_muted = FALSE;
	HrIfFailledThrow(simple_audio_volume_->GetMute(&is_muted));
	return is_muted;
}

uint32_t SharedWasapiDevice::GetVolume() const {	
	float channel_volume = 0.0;
	HrIfFailledThrow(simple_audio_volume_->GetMasterVolume(&channel_volume));
	return static_cast<uint32_t>(channel_volume * 100);
}

void SharedWasapiDevice::SetMute(bool mute) const {
	HrIfFailledThrow(simple_audio_volume_->SetMute(mute, nullptr));
}

PackedFormat SharedWasapiDevice::GetPackedFormat() const noexcept {
	return PackedFormat::INTERLEAVED;
}

uint32_t SharedWasapiDevice::GetBufferSize() const noexcept {
	return buffer_frames_ * mix_format_->nChannels;
}

void SharedWasapiDevice::SetSchedulerService(std::wstring const & mmcss_name, MmcssThreadPriority thread_priority) {
	XAMP_EXPECTS(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void SharedWasapiDevice::SetVolume(uint32_t volume) const {
	volume = std::clamp(volume, static_cast<uint32_t>(0), static_cast<uint32_t>(100));

	BOOL is_mute = FALSE;
	HrIfFailledThrow(simple_audio_volume_->GetMute(&is_mute));

	if (is_mute) {
		HrIfFailledThrow(simple_audio_volume_->SetMute(false, nullptr));
	}

	const auto channel_volume = static_cast<float>(static_cast<double>(volume) / 100.0);
	HrIfFailledThrow(simple_audio_volume_->SetMasterVolume(channel_volume, nullptr));

	XAMP_LOG_D(logger_, "Current volume: {}", GetVolume());
}

void SharedWasapiDevice::SetStreamTime(double stream_time) noexcept {
	stream_time_ = stream_time * static_cast<double>(mix_format_->nSamplesPerSec);
}

double SharedWasapiDevice::GetStreamTime() const noexcept {
	return stream_time_ / static_cast<double>(mix_format_->nSamplesPerSec);
}

void SharedWasapiDevice::ReportError(HRESULT hr) {
	if (FAILED(hr)) {
		const ComException exception(hr);
		callback_->OnError(exception);
		is_running_ = false;
	}
}

HRESULT SharedWasapiDevice::GetSample(uint32_t frame_available, bool is_silence) noexcept {
	XAMP_EXPECTS(render_client_ != nullptr);
	XAMP_EXPECTS(callback_ != nullptr);

	// Calculate stream time.
	const double stream_time = stream_time_ + frame_available;
	stream_time_ = stream_time;
	const auto stream_time_float = stream_time / static_cast<double>(mix_format_->nSamplesPerSec);

	DWORD flags = is_silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

	BYTE* data = nullptr;
	auto hr = render_client_->GetBuffer(frame_available, &data);
	if (FAILED(hr)) {
		return hr;
	}

	// Calculate sample time.
	const auto sample_time = GetStreamPosInMilliseconds(clock_) / 1000.0;	

	size_t num_filled_frames = 0;

	// Get sample from callback.
	XAMP_LIKELY(callback_->OnGetSamples(data, frame_available, num_filled_frames, stream_time_float, sample_time) == DataCallbackResult::CONTINUE) {
		if (num_filled_frames != frame_available) {
			flags = AUDCLNT_BUFFERFLAGS_SILENT;
		}
		hr = render_client_->ReleaseBuffer(frame_available, flags);
	}
	else {
		hr = render_client_->ReleaseBuffer(frame_available, AUDCLNT_BUFFERFLAGS_SILENT);
	}
	return hr;
}

HRESULT SharedWasapiDevice::OnInvoke(IMFAsyncResult *) {
	if (is_running_ && rt_work_queue_ != nullptr) {
		try {
			GetSample(false);
			rt_work_queue_->WaitAsync(sample_ready_.get());
		} catch (const Exception &e) {
			XAMP_LOG_D(logger_, e.what());
			StopStream();
		}
	}
	return S_OK;
}

void SharedWasapiDevice::StartStream() {
	XAMP_LOG_D(logger_, "StartStream!");

	if (!client_) {
		throw ComException(AUDCLNT_E_NOT_INITIALIZED);
	}

	// Note: 必要! 某些音效卡會爆音!
	GetSample(true);
	rt_work_queue_->Initial();
	rt_work_queue_->WaitAsync(sample_ready_.get());
	is_running_ = true;
	HrIfFailledThrow(client_->Start());
}

bool SharedWasapiDevice::IsStreamRunning() const noexcept {
	return is_running_;
}

HRESULT SharedWasapiDevice::GetSample(bool is_silence) noexcept {
	uint32_t padding_frames = 0;

	const auto hr = client_->GetCurrentPadding(&padding_frames);
	if (FAILED(hr)) {
		return hr;
	}

	const auto frames_available = buffer_frames_ - padding_frames;

	if (frames_available > 0) {
		if (is_silence) {
			return GetSample(frames_available, true);
		}
		else {
			return GetSample(frames_available, false);
		}
	}
	return S_OK;
}

void SharedWasapiDevice::AbortStream() noexcept {
	is_running_ = false;
}

bool SharedWasapiDevice::IsHardwareControlVolume() const {
	return true;
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif