#include <base/str_utilts.h>
#include <base/platform.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/stl.h>
#include <base/threadpoolexecutor.h>
#include <base/dsdsampleformat.h>
#include <base/buffer.h>
#include <base/timer.h>
#include <base/scopeguard.h>
#include <base/waitabletimer.h>
#include <base/executor.h>

#include <output_device/api.h>
#include <output_device/win32/asiodevicetype.h>
#include <output_device/idsddevice.h>
#include <output_device/iaudiodevicemanager.h>

#include <stream/api.h>
#include <stream/dspmanager.h>
#include <stream/iaudiostream.h>
#include <stream/idsdstream.h>
#include <stream/filestream.h>
#include <stream/bassfader.h>
#include <stream/compressorparameters.h>
#include <stream/iaudioprocessor.h>

#include <player/iplaybackstateadapter.h>
#include <player/audio_player.h>

namespace xamp::player {

XAMP_DECLARE_LOG_NAME(AudioPlayer);

#ifdef _DEBUG
inline constexpr int32_t kBufferStreamCount = 3;
#else
inline constexpr int32_t kBufferStreamCount = 2;
#endif

inline constexpr int32_t kTotalBufferStreamCount = 16;

inline constexpr uint32_t kPreallocateBufferSize = 4 * 1024 * 1024;
inline constexpr uint32_t kMaxPreAllocateBufferSize = 32 * 1024 * 1024;

inline constexpr uint32_t kMaxBufferSecs = 5;
	
inline constexpr uint32_t kMaxWriteRatio = 64;
inline constexpr uint32_t kMaxReadRatio = 10;
inline constexpr uint32_t kActionQueueSize = 30;

inline constexpr size_t kFFTSize = 512;

inline constexpr std::chrono::milliseconds kUpdateSampleInterval(100);

inline constexpr std::chrono::milliseconds kReadSampleWaitTime(30);
inline constexpr std::chrono::milliseconds kPauseWaitTimeout(30);
inline constexpr std::chrono::seconds kWaitForStreamStopTime(10);
inline constexpr std::chrono::seconds kWaitForSignalWhenReadFinish(3);

static AlignPtr<FileStream> MakeFileStream(DsdModes dsd_mode) {
    auto file_stream = StreamFactory::MakeFileStream(dsd_mode);

    /*if (auto* dsd_stream = AsDsdStream(file_stream)) {
        switch (dsd_mode) {
        case DsdModes::DSD_MODE_DOP:
            if (!dsd_stream->SupportDOP()) {
                throw NotSupportFormatException();
            }
            break;
        case DsdModes::DSD_MODE_DOP_AA:
            if (!dsd_stream->SupportDOP_AA()) {
                throw NotSupportFormatException();
            }
            break;
        case DsdModes::DSD_MODE_NATIVE:
            if (!dsd_stream->SupportNativeSD()) {
                throw NotSupportFormatException();
            }
            break;
        case DsdModes::DSD_MODE_DSD2PCM:
            break;
        default:
            throw NotSupportFormatException();
        }
        dsd_stream->SetDSDMode(dsd_mode);
    }*/
    return file_stream;
}

#ifdef ENABLE_ASIO
IDsdDevice* AsDsdDevice(AlignPtr<IOutputDevice> const& device) noexcept {
    return dynamic_cast<IDsdDevice*>(device.get());
}
#endif

AudioPlayer::AudioPlayer()
    : is_muted_(false)
	, is_dsd_file_(false)
    , enable_fadeout_(false)
    , dsd_mode_(DsdModes::DSD_MODE_PCM)
    , sample_size_(0)
    , target_sample_rate_(0)
    , fifo_size_(0)
    , num_read_sample_(0)
    , volume_(0)
    , is_fade_out_(false)
    , is_playing_(false)
    , is_paused_(false)
    , playback_progress_{-1}
    , state_(PlayerState::PLAYER_STATE_STOPPED)
    , sample_end_time_(0)
    , stream_duration_(0)
    , device_manager_(MakeAudioDeviceManager())
    , fifo_(GetPageAlignSize(kPreallocateBufferSize))
    , action_queue_(kActionQueueSize)
    , dsp_manager_(StreamFactory::MakeDSPManager()) {
    logger_ = LoggerManager::GetInstance().GetLogger(kAudioPlayerLoggerName);
}

AudioPlayer::~AudioPlayer() = default;

void AudioPlayer::Destroy() {
    timer_.Stop();
    try {
        CloseDevice(true, true);
    }
    catch (...) {
    }
    Stop(false, true);
    stream_.reset();
    read_buffer_.reset();
#ifdef ENABLE_ASIO
    ResetAsioDriver();
#endif

    GetPlaybackThreadPool().Stop();
#ifdef XAMP_OS_WIN
    GetWasapiThreadPool().Stop();
#endif
    PreventSleep(false);
}

void AudioPlayer::Startup(const std::weak_ptr<IPlaybackStateAdapter>& adapter) {
    PreventSleep(true);

    state_adapter_ = adapter;
    device_manager_->RegisterDeviceListener(shared_from_this());

    std::weak_ptr<AudioPlayer> player = shared_from_this();
    timer_.Start(kUpdateSampleInterval, [player]() {
        auto p = player.lock();
        if (!p) {
            return;
        }

        const auto adapter = p->state_adapter_.lock();
        if (!adapter) {
            return;
        }

        if (p->is_paused_) {
            return;
        }
        if (!p->is_playing_) {
            return;
        }

        const auto playback_event = p->playback_progress_.load();
        if (playback_event > 0) {
            adapter->OnSampleTime(p->device_->GetStreamTime());
        }
        else if (p->is_playing_ && playback_event == -1) {
            p->SetState(PlayerState::PLAYER_STATE_STOPPED);
            p->is_playing_ = false;
        }
    });
}

void AudioPlayer::Open(Path const& file_path, const Uuid& device_id) {
    AlignPtr<IDeviceType> device_type;
    if (device_id.IsValid()) {
        device_type = device_manager_->CreateDefaultDeviceType();
    } else {
        device_type = device_manager_->Create(device_id);
    }

    device_type->ScanNewDevice();
    if (const auto device_info = device_type->GetDefaultDeviceInfo()) {
        Open(file_path, *device_info);
    } else {
        throw DeviceNotFoundException();
    }
}

void AudioPlayer::Open(Path const& file_path, const DeviceInfo& device_info, uint32_t target_sample_rate, DsdModes output_mode) {
    CloseDevice(true);
    UpdateProgress();
    OpenStream(file_path, output_mode);
    device_info_ = device_info;
    target_sample_rate_ = target_sample_rate;
    XAMP_LOG_D(logger_,
        "Deveice min_volume: {:.2f} dBFS, max_volume:{:.2f} dBFS, volume_increnment:{:.2f} dBFS, volume leve:{:.2f}.",
        device_info_.min_volume,
        device_info_.max_volume,
        device_info_.volume_increment,
        (device_info_.max_volume - device_info_.min_volume) / device_info_.volume_increment);
}

void AudioPlayer::SetDevice(const DeviceInfo& device_info) {
    device_info_ = device_info;
}

DeviceInfo AudioPlayer::GetDevice() const {
    return device_info_;
}

void AudioPlayer::CreateDevice(Uuid const & device_type_id, std::string const & device_id, bool open_always) {
    if (device_ == nullptr
        || device_id_ != device_id
        || device_type_id_ != device_type_id
        || open_always) {
        if (device_type_id_ != device_type_id) {
            // device可能是ASIO解後再移除drvier.
            device_.reset();
            ResetAsioDriver();
            XAMP_LOG_D(logger_, "ResetASIODriver!");
        }    	
        device_type_ = device_manager_->Create(device_type_id);
        device_type_->ScanNewDevice();
        device_ = device_type_->MakeDevice(device_id);
        device_type_id_ = device_type_id;
        device_id_ = device_id;
        XAMP_LOG_D(logger_, "Create device: {}", device_type_->GetDescription());
    }
    device_->SetAudioCallback(this);
}

bool AudioPlayer::IsDsdFile() const {
    return is_dsd_file_;
}

void AudioPlayer::SetStreamInfo(DsdModes dsd_mode, AlignPtr<FileStream>& stream) {
    dsd_mode_ = dsd_mode;

    stream_duration_ = stream->GetDuration();
    input_format_ = stream->GetFormat();

    if (dsd_mode == DsdModes::DSD_MODE_PCM) {
        dsd_speed_ = std::nullopt;
        return;
    }

    if (const auto* dsd_stream = AsDsdStream(stream)) {
        if (!dsd_stream->IsDsdFile()) {
            return;
        }
        is_dsd_file_ = true;
        dsd_speed_ = dsd_stream->GetDsdSpeed();
    } else {
        is_dsd_file_ = false;
        dsd_speed_ = std::nullopt;
    }
}

void AudioPlayer::OpenStream(Path const& file_path, DsdModes dsd_mode) {
    stream_ = MakeFileStream(dsd_mode);
    stream_->OpenFile(file_path);
    SetStreamInfo(dsd_mode, stream_);
    XAMP_LOG_D(logger_, "Open stream type: {} {} duration:{:.2f} sec.",
        stream_->GetDescription(),
        dsd_mode_,
        stream_duration_.load());
}

void AudioPlayer::SetState(const PlayerState play_state) {
    if (const auto adapter = state_adapter_.lock()) {
        adapter->OnStateChanged(play_state);
    }
    state_ = play_state;
    XAMP_LOG_D(logger_, "Set state: {}.", EnumToString(state_));
}

void AudioPlayer::ReadPlayerAction() {
    while (!action_queue_.empty()) {
        if (const auto* msg = action_queue_.Front()) {
            try {
                XAMP_ON_SCOPE_EXIT({
                action_queue_.Pop();
                    });
                switch (msg->id) {
                case PlayerActionId::PLAYER_SEEK:
                {
                    auto stream_time = std::any_cast<double>(msg->content);
                    XAMP_LOG_D(logger_, "Receive seek {:.2f} message.", stream_time);
                    DoSeek(stream_time);
                }
                break;
                case PlayerActionId::PLAYER_SOFTWARE_VOLUME:
	                {
						auto volume_db = std::any_cast<double>(msg->content);
						XAMP_LOG_D(logger_, "Receive change volume {:.2f} dB.", volume_db);
                        config_.AddOrReplace(DspConfig::kVolume, volume_db);
                        dsp_manager_->Init(config_);
	                }
                    break;
                }
            } catch (std::exception const &e) {
                XAMP_LOG_D(logger_, "Receive {} {}.", EnumToString(msg->id), e.what());
            }
        }
	}    
}
	
void AudioPlayer::Pause() {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player pasue.");
    if (!is_paused_) {        
        if (device_->IsStreamOpen()) {
            is_paused_ = true;
            device_->StopStream(false);
            SetState(PlayerState::PLAYER_STATE_PAUSED);            
        }
    }
}

void AudioPlayer::Resume() {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player resume.");
    if (device_->IsStreamOpen()) {
        SetState(PlayerState::PLAYER_STATE_RESUME);
        is_paused_ = false;
        pause_cond_.notify_all();
        read_finish_and_wait_seek_signal_cond_.notify_all();
        device_->StartStream();
        SetState(PlayerState::PLAYER_STATE_RUNNING);
    }
}

void AudioPlayer::FadeOut() {
	const float kFadeTimeSeconds = 1.0;
    const auto sample_count = output_format_.GetSecondsSize(kFadeTimeSeconds) / sizeof(float);

    Buffer<float> buffer(sample_count);
    size_t num_filled_count = 0;
    dynamic_cast<BassFader*>(fader_.get())->SetTime(1.0f, 0.0f, kFadeTimeSeconds);

    if (!fifo_.TryRead(reinterpret_cast<int8_t*>(buffer.data()), buffer.GetByteSize(), num_filled_count)) {
        return;
    }

    Buffer<float> fade_buf(sample_count);
    BufferRef<float> buf_ref(fade_buf);
    fader_->Process(buffer.data(), buffer.size(), buf_ref);

    fifo_.Clear();
    fifo_.TryWrite(reinterpret_cast<int8_t*>(buf_ref.data()), buf_ref.GetByteSize());
}

void AudioPlayer::ProcessFadeOut() {
    if (!device_) {
        return;
    }
    if (!GetDspManager()->IsEnablePcm2DsdConverter()) {
        if (dsd_mode_ == DsdModes::DSD_MODE_PCM
            || dsd_mode_ == DsdModes::DSD_MODE_DSD2PCM) {
            XAMP_LOG_D(logger_, "Process fadeout.");
            is_fade_out_ = true;
            MSleep(std::chrono::milliseconds(1000));
        }
    }
}

void AudioPlayer::Stop(bool signal_to_stop, bool shutdown_device, bool wait_for_stop_stream) {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player stop.");
    if (device_->IsStreamOpen()) {
        XAMP_LOG_D(logger_, "Close device.");
        CloseDevice(wait_for_stop_stream);
        UpdateProgress();
        if (signal_to_stop) {
            SetState(PlayerState::PLAYER_STATE_USER_STOPPED);                        
        }
    }

    if (shutdown_device) {
        XAMP_LOG_D(logger_, "Shutdown device.");
        if (IsAsioDevice(device_type_id_)) {
            device_.reset();
            ResetAsioDriver();            
        }
        device_id_.clear();
        device_.reset();
    }
    stream_.reset();
    fifo_.Clear();
}

void AudioPlayer::SetVolume(uint32_t volume) {
    volume_ = volume;
    if (!device_ || !device_->IsStreamOpen()) {
        return;
    }
    device_->SetVolume(volume);
}

void AudioPlayer::SetSoftwareVolumeDb(double volume_db) {
    if (!device_) {
        return;
    }

    if (device_->IsStreamOpen()) {
        read_finish_and_wait_seek_signal_cond_.notify_one();
        action_queue_.TryPush(PlayerAction{
            PlayerActionId::PLAYER_SOFTWARE_VOLUME,
            volume_db
            });
    }
}

uint32_t AudioPlayer::GetVolume() const {
    if (!device_ || !device_->IsStreamOpen()) {
        return volume_;
    }
    return device_->GetVolume();
}

bool AudioPlayer::IsHardwareControlVolume() const {
    if (device_ != nullptr && device_->IsStreamOpen()) {
        return device_->IsHardwareControlVolume();
    }
    return true;
}

bool AudioPlayer::IsMute() const {
    if (device_ != nullptr && device_->IsStreamOpen()) {
#ifdef ENABLE_ASIO
        if (device_type_->GetTypeId() == XAMP_UUID_OF(win32::AsioDeviceType)) {
            return is_muted_;
        }
#else
        return device_->IsMuted();
#endif
    }
    return is_muted_;
}

void AudioPlayer::SetMute(bool mute) {
    is_muted_ = mute;
    if (!device_ || !device_->IsStreamOpen()) {
        return;
    }
    device_->SetMute(mute);
}

std::optional<uint32_t> AudioPlayer::GetDsdSpeed() const {
    return dsd_speed_;	
}

double AudioPlayer::GetStreamTime() const {
    if (!device_) {
        return 0.0;
    }
    return device_->GetStreamTime();
}

double AudioPlayer::GetDuration() const {
    if (!stream_) {
        return 0.0;
    }
    return stream_duration_;
}

PlayerState AudioPlayer::GetState() const noexcept {
    return state_;
}

AudioFormat AudioPlayer::GetInputFormat() const noexcept {
    auto file_format = input_format_;
    file_format.SetBitPerSample(stream_->GetBitDepth());
    return file_format;
}

AudioFormat AudioPlayer::GetOutputFormat() const noexcept {
    return output_format_;
}

bool AudioPlayer::IsPlaying() const noexcept {
    return is_playing_;
}

DsdModes AudioPlayer::GetDsdModes() const noexcept {
    return dsd_mode_;
}

void AudioPlayer::CloseDevice(bool wait_for_stop_stream, bool quit) {
    is_playing_ = false;
    is_paused_ = false;
    pause_cond_.notify_all();
    read_finish_and_wait_seek_signal_cond_.notify_all();

    if (stream_task_.valid()) {
        XAMP_LOG_D(logger_, "Try to stop stream thread.");
#ifdef XAMP_OS_WIN
        // MSVC 2019 is wait for std::packaged_task return timeout, while others such clang can't.
        if (stream_task_.wait_for(kWaitForStreamStopTime) == std::future_status::timeout) {
            throw StopStreamTimeoutException();
        }
#else
        stream_task_.get();
#endif
        stream_task_ = Task<void>();
        XAMP_LOG_D(logger_, "Stream thread was finished.");
    }

    if (!quit && enable_fadeout_) {
        ProcessFadeOut();
    }

    if (device_ != nullptr) {
        if (device_->IsStreamOpen()) {
            XAMP_LOG_D(logger_, "Stop output device");
            try {
                device_->StopStream(wait_for_stop_stream);
                device_->CloseStream();
			}
			catch (const Exception& e) {
				XAMP_LOG_D(logger_, "Close device failure. {}", e.what());
			}
		}
	}

    fifo_.Clear();
    action_queue_.clear();
}

void AudioPlayer::AllocateReadBuffer(uint32_t allocate_size) {
    if (read_buffer_.GetSize() == 0 || read_buffer_.GetSize() != allocate_size) {
        XAMP_LOG_D(logger_, "Allocate read buffer : {}.", String::FormatBytes(allocate_size));
        read_buffer_ = MakeBuffer<int8_t>(allocate_size);
    }
}

void AudioPlayer::AllocateFifo() {
    if (fifo_.GetSize() == 0 || fifo_.GetSize() < fifo_size_) {
        XAMP_LOG_D(logger_, "Allocate fifo buffer : {}.", String::FormatBytes(fifo_size_));
        fifo_.Resize(fifo_size_);
    }
}

void AudioPlayer::CreateBuffer() {
    uint32_t require_read_sample = 0;

    if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
        require_read_sample = static_cast<uint32_t>(
            GetPageAlignSize(output_format_.GetSampleRate() / 8));
        num_read_sample_ = require_read_sample;
    } else {
	    const auto ratio = (std::max)(kMaxReadRatio, kMaxWriteRatio);
        require_read_sample = static_cast<uint32_t>(
            GetPageAlignSize(device_->GetBufferSize() * output_format_.GetChannels() * ratio));
        num_read_sample_ = static_cast<uint32_t>(
            GetPageAlignSize(device_->GetBufferSize() * output_format_.GetChannels() * kMaxReadRatio));
    }

    uint32_t allocate_read_size = 0;
    if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
        allocate_read_size = kMaxPreAllocateBufferSize;
    } else {
        allocate_read_size = (std::min)(
            kMaxPreAllocateBufferSize,
            require_read_sample *
            stream_->GetSampleSize() * 
            kBufferStreamCount);
        allocate_read_size = AlignUp(allocate_read_size);
    }

    fifo_size_ = (std::max)(
        output_format_.GetAvgBytesPerSec() * kMaxBufferSecs,
        allocate_read_size * kTotalBufferStreamCount);

    AllocateReadBuffer(allocate_read_size);
    AllocateFifo();

    XAMP_LOG_D(logger_, "Read memory page count: {} page.", num_read_sample_ / GetPageSize());

    XAMP_LOG_D(logger_, "Output buffer:{} device format: {} num_read_sample: {} fifo buffer: {}.",
        device_->GetBufferSize(),
        output_format_,
        String::FormatBytes(num_read_sample_),
        String::FormatBytes(fifo_.GetSize()));
}

void AudioPlayer::SetDeviceFormat() {
    if (target_sample_rate_ != 0 && CanConverter()) {
        if (output_format_.GetSampleRate() != target_sample_rate_) {
            device_id_.clear();
        }
        output_format_ = input_format_;
        output_format_.SetSampleRate(target_sample_rate_);
    }
    else {
        if (input_format_.GetSampleRate() != output_format_.GetSampleRate()) {
            device_id_.clear();
        }
        output_format_ = input_format_;
    }
}

void AudioPlayer::OnVolumeChange(float vol) noexcept {
    if (const auto adapter = state_adapter_.lock()) {
        adapter->OnVolumeChanged(vol);
        XAMP_LOG_D(logger_, "Volum change: {}.", vol);
    }
}

void AudioPlayer::OnError(const Exception& e) noexcept {
    is_playing_ = false;
    XAMP_LOG_DEBUG(e.what());
}

void AudioPlayer::OnDeviceStateChange(DeviceState state, std::string const & device_id) {    
    if (const auto state_adapter = state_adapter_.lock()) {
        switch (state) {
        case DeviceState::DEVICE_STATE_ADDED:
            XAMP_LOG_D(logger_, "Device added device id:{}.", device_id);
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_ADDED);
            break;
        case DeviceState::DEVICE_STATE_REMOVED:
            XAMP_LOG_D(logger_, "Device removed device id:{}.", device_id);
            if (device_id == device_id_) {
                // TODO: In many system has more ASIO device.
                if (IsAsioDevice(device_type_->GetTypeId())) {
                    ResetAsioDriver();
                }
                
                state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_REMOVED);
                if (device_ != nullptr) {
                    device_->AbortStream();
                    XAMP_LOG_D(logger_, "Device abort stream id:{}.", device_id);
                }
            }
            break;
        case DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE:
            XAMP_LOG_D(logger_, "Default device device id:{}.", device_id);
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE);
            break;
        }
    }
}

void AudioPlayer::OpenDevice(double stream_time) {
#ifdef ENABLE_ASIO
    if (auto* dsd_output = AsDsdDevice(device_)) {
        if (dsd_mode_ == DsdModes::DSD_MODE_AUTO || dsd_mode_ == DsdModes::DSD_MODE_PCM) {
            if (const auto* const dsd_stream = AsDsdStream(stream_)) {
                if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE || dsd_mode_ == DsdModes::DSD_MODE_DOP) {
                    dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_DSD);
                }
                else {
                    dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_PCM);
                }
            }
        } else {
            output_format_.SetFormat(DataFormat::FORMAT_DSD);
            output_format_.SetByteFormat(ByteFormat::SINT8);
            dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_DSD);
        }
    }
#endif
    device_->OpenStream(output_format_);
    device_->SetStreamTime(stream_time);
}

void AudioPlayer::BufferStream(double stream_time) {
    XAMP_LOG_D(logger_, "Buffing samples : {:.2f}msec", stream_time);

    fifo_.Clear();
    stream_->Seek(stream_time);
    sample_size_ = stream_->GetSampleSize();    
    BufferSamples(stream_, kBufferStreamCount);
}

void AudioPlayer::EnableFadeOut(bool enable) {
    enable_fadeout_ = enable;
}

void AudioPlayer::UpdateProgress(int32_t sample_size) noexcept {
    playback_progress_.exchange(sample_size);
}

void AudioPlayer::Seek(double stream_time) {
    if (!device_) {
        return;
    }

    if (device_->IsStreamOpen()) {
        read_finish_and_wait_seek_signal_cond_.notify_one();
        action_queue_.TryPush(PlayerAction{
            PlayerActionId::PLAYER_SEEK,
            stream_time
            });
    }
}

void AudioPlayer::DoSeek(double stream_time) {
	if (state_ != PlayerState::PLAYER_STATE_PAUSED) {
        Pause();
	}
	
    try {
        stream_->Seek(stream_time);
    }
    catch (Exception const& e) {
        XAMP_LOG_D(logger_, e.GetErrorMessage());
        Resume();
        return;
    }

    device_->SetStreamTime(stream_time);
    sample_end_time_ = stream_->GetDuration() - stream_time;
    XAMP_LOG_D(logger_, "Stream duration:{:.2f} seeking:{:.2f} sec, end time:{:.2f} sec.",
        stream_->GetDuration(),
        stream_time,
        sample_end_time_);
    UpdateProgress();
    fifo_.Clear();
    BufferStream(stream_time);
    Resume();
}

bool AudioPlayer::CanConverter() const noexcept {
    return (dsd_mode_ == DsdModes::DSD_MODE_PCM || dsd_mode_ == DsdModes::DSD_MODE_DSD2PCM);
}

void AudioPlayer::BufferSamples(const AlignPtr<FileStream>& stream, int32_t buffer_count) {
    auto* const sample_buffer = read_buffer_.Get();

    for (auto i = 0; i < buffer_count && stream_->IsActive(); ++i) {
        XAMP_LOG_D(logger_, "Buffering {} ...", i);

        while (true) {
            const auto num_samples = stream->GetSamples(sample_buffer, num_read_sample_);
            if (num_samples == 0) {
                return;
            }

            auto* samples = reinterpret_cast<const float*>(sample_buffer);
            if (dsp_manager_->ProcessDSP(samples, num_samples, fifo_)) {
                continue;
            }            
            break;
        }
    }
}

void AudioPlayer::ReadSampleLoop(int8_t *sample_buffer, uint32_t max_buffer_sample, std::unique_lock<FastMutex>& stopped_lock) {
    if (!stream_->IsActive()) {
        if (is_playing_) {
            if (read_finish_and_wait_seek_signal_cond_.wait_for(stopped_lock, kWaitForSignalWhenReadFinish)
                != std::cv_status::timeout) {
                XAMP_LOG_D(logger_, "Stream is read done!, Weak up for seek signal.");
            }
        }
    	return;
    }

    while (is_playing_ && stream_->IsActive()) {
        const auto num_samples = stream_->GetSamples(sample_buffer, max_buffer_sample);

        if (num_samples > 0) {
            auto *samples = reinterpret_cast<const float*>(sample_buffer);

            if (dsp_manager_->ProcessDSP(samples, num_samples, fifo_)) {
                continue;
            }
        }
        break;
    }
}

const AlignPtr<IAudioDeviceManager>& AudioPlayer::GetAudioDeviceManager() {
    return device_manager_;
}

AlignPtr<IDSPManager>& AudioPlayer::GetDspManager() {
    return dsp_manager_;
}

void AudioPlayer::SetReadSampleSize(uint32_t num_samples) {
    num_read_sample_ = num_samples;
    XAMP_LOG_D(logger_, "Output buffer:{} device format: {} num_read_sample: {} fifo buffer: {}.",
               device_->GetBufferSize(),
               output_format_,
               String::FormatBytes(num_read_sample_),
               String::FormatBytes(fifo_.GetSize()));
}

void AudioPlayer::Play() {
    if (!device_) {
        return;
    }

    if (const auto state = state_adapter_.lock()) {
        state->OutputFormatChanged(output_format_, device_->GetBufferSize());
    }

    is_fade_out_ = false;
    is_playing_ = true;
    if (device_->IsStreamOpen()) {
        if (!device_->IsStreamRunning()) {
            XAMP_LOG_D(logger_, "Play volume:{} muted:{}.", volume_, is_muted_);
            device_->StartStream();
            SetState(PlayerState::PLAYER_STATE_RUNNING);
        }
    }

    if (stream_task_.valid()) {
        return;
    }

    stream_task_ = Executor::Spawn(GetPlaybackThreadPool(), [player = shared_from_this()]() {
        auto* p = player.get();

        std::unique_lock<FastMutex> pause_lock{ p->pause_mutex_ };
        std::unique_lock<FastMutex> stopped_lock{ p->stopped_mutex_ };

        auto* sample_buffer = p->read_buffer_.Get();
        const auto max_buffer_sample = p->num_read_sample_;
        const auto num_sample_write = max_buffer_sample * kMaxWriteRatio;

        XAMP_LOG_D(p->logger_, "max_buffer_sample: {}, num_sample_write: {}",
            String::FormatBytes(max_buffer_sample),
            String::FormatBytes(num_sample_write)
        );

        WaitableTimer wait_timer;
        wait_timer.SetTimeout(kReadSampleWaitTime);

        try {
            while (p->is_playing_) {
                while (p->is_paused_) {
                    p->pause_cond_.wait_for(pause_lock, kPauseWaitTimeout);
                    p->ReadPlayerAction();
                }

                p->ReadPlayerAction();

                if (p->fifo_.GetAvailableWrite() < num_sample_write) {
                    wait_timer.Wait();
                    XAMP_LOG_T(p->logger_, "FIFO buffer: {} num_sample_write: {}",
                               p->fifo_.GetAvailableWrite(),
                               num_sample_write
                               );
                    continue;
                }                
                p->ReadSampleLoop(sample_buffer, max_buffer_sample, stopped_lock);
            }
        }
        catch (const Exception& e) {
            XAMP_LOG_D(p->logger_, "Stream thread read has exception: {}.", e.what());
        }
        catch (const std::exception& e) {
            XAMP_LOG_D(p->logger_, "Stream thread read has exception: {}.", e.what());
        }

        XAMP_LOG_D(p->logger_, "Stream thread done!");
        p->stream_.reset();
    });
}

void AudioPlayer::CopySamples(void* samples, size_t num_buffer_frames) const {
    if (!CanConverter()) {
        return;
    }

    const auto adapter = state_adapter_.lock();
    if (!adapter) {
        return;
    }
    adapter->OnSamplesChanged(static_cast<const float*>(samples), num_buffer_frames);
}

DataCallbackResult AudioPlayer::OnGetSamples(void* samples, size_t num_buffer_frames, size_t & num_filled_frames, double stream_time, double /*sample_time*/) noexcept {
    const auto num_samples = num_buffer_frames * output_format_.GetChannels();
    const auto sample_size = num_samples * sample_size_;

    if (is_fade_out_) {
        FadeOut();
        is_fade_out_ = false;
    }

    size_t num_filled_bytes = 0;
    XAMP_LIKELY(fifo_.TryRead(static_cast<int8_t*>(samples), sample_size, num_filled_bytes)) {
        num_filled_frames = num_filled_bytes / sample_size_ / output_format_.GetChannels();
        num_filled_frames = num_buffer_frames;
        UpdateProgress(static_cast<int32_t>(num_samples));
        CopySamples(samples, num_samples);
        return DataCallbackResult::CONTINUE;
    }

    // note: 為了避免提早將聲音切斷(某些音效介面固定frame大小,低於某個frame大小就無法撥放 ex:WASAPI)
    // 下次取frame的時候才將聲音停止.
    // (WASAPI 1 frame)  (WASAPI 2 frame)
    //       |                 |
    //       | End time        |
    //       V    V            V
    // <------------------------------->
    //
    if (stream_time >= stream_duration_) {
        UpdateProgress(-1);
        return DataCallbackResult::STOP;
    }

    num_filled_frames = num_buffer_frames;
    UpdateProgress(static_cast<int32_t>(num_samples));
    return DataCallbackResult::CONTINUE;
}

void AudioPlayer::PrepareToPlay(ByteFormat byte_format, uint32_t device_sample_rate) {
    if (device_sample_rate != 0) {
        target_sample_rate_ = device_sample_rate;
    }

    SetDeviceFormat();

	if (byte_format != ByteFormat::INVALID_FORMAT) {
        output_format_.SetByteFormat(byte_format);
    }

    CreateDevice(device_info_.device_type_id, device_info_.device_id, false);
    OpenDevice(0);
    CreateBuffer();

    config_.AddOrReplace(DspConfig::kInputFormat, std::any(input_format_));
    config_.AddOrReplace(DspConfig::kOutputFormat, std::any(output_format_));
    config_.AddOrReplace(DspConfig::kDsdMode, std::any(dsd_mode_));
    config_.AddOrReplace(DspConfig::kSampleSize, std::any(stream_->GetSampleSize()));
    config_.AddOrReplace(DspConfig::kCompressorParameters, std::any(CompressorParameters()));
    config_.AddOrReplace(DspConfig::kVolume, std::any(1.0));

    dsp_manager_->Init(config_);
	sample_end_time_ = stream_->GetDuration();
    XAMP_LOG_D(logger_, "Stream end time: {:.2f} sec.", sample_end_time_);
    fader_ = StreamFactory::MakeFader();
    fader_->Start(config_);
}

AnyMap& AudioPlayer::GetDspConfig() {
    return config_;
}

}
