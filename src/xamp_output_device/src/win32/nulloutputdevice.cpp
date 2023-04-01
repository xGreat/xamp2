#include <base/executor.h>
#include <base/ithreadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/timer.h>
#include <output_device/win32/nulloutputdevice.h>

#include "output_device/iaudiocallback.h"

#ifdef XAMP_OS_WIN

namespace xamp::output_device::win32 {

NullOutputDevice::NullOutputDevice()
	: is_running_(false)
	, is_muted_(false)
	, is_stopped_(true)
	, buffer_frames_(0)
	, callback_(nullptr)
	, log_(LoggerManager::GetInstance().GetLogger(kNullOutputDeviceLoggerName)) {
}

NullOutputDevice::~NullOutputDevice() {
}

bool NullOutputDevice::IsStreamOpen() const noexcept {
	return true;
}

void NullOutputDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
	callback_ = callback;
}

void NullOutputDevice::StopStream(bool wait_for_stop_stream) {
	if (!is_running_) {
		return;
	}

	is_stopped_ = true;
	if (render_task_.valid()) {
		render_task_.get();
	}
	is_running_ = false;
}

void NullOutputDevice::CloseStream() {
	render_task_ = Task<void>();
}

void NullOutputDevice::OpenStream(AudioFormat const & output_format) {
	buffer_frames_ = 432;
	const size_t buffer_size = buffer_frames_ * output_format.GetChannels();
	if (buffer_.size() != buffer_size) {
		buffer_ = MakeBuffer<float>(buffer_size);
	}
	output_format_ = output_format;
}

bool NullOutputDevice::IsMuted() const {
	return is_muted_;
}

uint32_t NullOutputDevice::GetVolume() const {
	return 0;
}

void NullOutputDevice::SetMute(bool mute) const {
	is_muted_ = mute;
}

PackedFormat NullOutputDevice::GetPackedFormat() const noexcept {
	return PackedFormat::INTERLEAVED;
}

uint32_t NullOutputDevice::GetBufferSize() const noexcept {
	return buffer_frames_ * AudioFormat::kMaxChannel;
}

void NullOutputDevice::SetVolume(uint32_t volume) const {

}

void NullOutputDevice::SetStreamTime(double stream_time) noexcept {
	stream_time_ = stream_time * static_cast<double>(output_format_.GetSampleRate());
}

double NullOutputDevice::GetStreamTime() const noexcept {
	return stream_time_ / static_cast<double>(output_format_.GetSampleRate());
}

void NullOutputDevice::StartStream() {
	is_stopped_ = false;
	render_task_ = Executor::Spawn(GetWasapiThreadPool(), [this]() {		
		size_t num_filled_frames = 0;		
		double sample_time = 0;

		while (!is_stopped_) {
			const auto stream_time = stream_time_ + buffer_frames_;
			stream_time_ = stream_time;
			const auto stream_time_float = static_cast<double>(stream_time) / output_format_.GetSampleRate();

			is_running_ = true;
			XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_frames_, num_filled_frames, stream_time_float, sample_time) == DataCallbackResult::CONTINUE) {

			} else {
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});
}

bool NullOutputDevice::IsStreamRunning() const noexcept {
	return is_running_;
}

void NullOutputDevice::AbortStream() noexcept {
}

bool NullOutputDevice::IsHardwareControlVolume() const {
	return true;
}

}
#endif
