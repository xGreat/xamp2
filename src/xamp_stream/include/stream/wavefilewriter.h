//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/fs.h>
#include <base/audioformat.h>

#include <fstream>

XAMP_STREAM_NAMESPACE_BEGIN

class WaveFileWriter final {
public:
	WaveFileWriter() = default;
	
	void Open(Path const& file_path, AudioFormat const& format);

	XAMP_DISABLE_COPY(WaveFileWriter)

	void Close();

	[[nodiscard]] bool TryWrite(float const* sample, size_t num_samples);

	void Write(float const* sample, size_t num_samples);
	
private:
	void WriteHeader(AudioFormat const& format);
	
	void WriteDataLength();

	void WriteSample(float sample);

	uint32_t data_length_{0};
	AudioFormat format_;
	std::ofstream file_;
};

XAMP_STREAM_NAMESPACE_END
