//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/buffer.h>
#include <base/task.h>

#ifdef XAMP_OS_WIN

#include <output_device/ioutputdevice.h>
#include <base/logger.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(NullOutputDevice);

/*
* NullOutputDevice is the null output device.
* 
*/
class NullOutputDevice final : public IOutputDevice {
public:
	/*
	* Constructor
	*/
	NullOutputDevice();

	/*
	* Destructor
	*/
	virtual ~NullOutputDevice() override;

	/*
	* Open stream
	*
	* @param output_format: output format
	* @return void
	*/
	void OpenStream(AudioFormat const & output_format) override;

	/*
	* Set audio callback
	* 
	* @param callback: audio callback
	*/
	void SetAudioCallback(IAudioCallback* callback) noexcept override;

	/*
	* Is stream open
	*
	* return bool
	*/
	bool IsStreamOpen() const noexcept override;

	/*
	* Is stream running
	*
	* @return bool
	*/
	bool IsStreamRunning() const noexcept override;

	/*
	* Stop stream
	*
	* @param[in] wait_for_stop_stream: wait for stop stream
	*/
	void StopStream(bool wait_for_stop_stream = true) override;

	/*
	* Close stream
	*
	*/
	void CloseStream() override;

	/*
	* Start stream
	*
	*/
	void StartStream() override;

	/*
	* Set stream time
	*
	* @param stream_time: stream time
	*/
	void SetStreamTime(double stream_time) noexcept override;

	/*
	* Get stream time
	*
	*/
	double GetStreamTime() const noexcept override;

	/*
	* Get volume
	*
	* @return uint32_t
	*/
	uint32_t GetVolume() const override;

	/*
	* Is muted
	*
	* @return bool
	*/
	bool IsMuted() const override;

	/*
	* Set volume
	* @param volume: volume (1~100)
	*/
	void SetVolume(uint32_t volume) const override;

	/*
	* Set mute
	*
	* @param[in] mute: mute (true/false)
	*/
	void SetMute(bool mute) const override;

	/*
	* Get packed format
	*
	* @return PackedFormat
	*/
	PackedFormat GetPackedFormat() const noexcept override;

	/*
	* Get device buffer size
	*
	* @return uint32_t
	*/
	uint32_t GetBufferSize() const noexcept override;

	/*
	* Is hardware control volume
	*
	* @return bool
	*/
	bool IsHardwareControlVolume() const override;

	/*
	* Abort stream
	*
	* @return void
	*/
	void AbortStream() noexcept override;

private:
	bool is_running_;
	mutable bool is_muted_;
	std::atomic<bool> is_stopped_;
	uint32_t buffer_frames_;
	std::atomic<int64_t> stream_time_;
	IAudioCallback* callback_;
	Buffer<float> buffer_;
	Task<void> render_task_;
	std::chrono::milliseconds wait_time_;
	AudioFormat output_format_;
	LoggerPtr log_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN


