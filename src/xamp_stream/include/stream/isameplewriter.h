//=====================================================================================================================
// Copyright (c) 2018-2021 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/audiobuffer.h>

namespace xamp::stream {

using namespace xamp::base;

class XAMP_STREAM_API XAMP_NO_VTABLE ISampleWriter {
public:
    XAMP_BASE_CLASS(ISampleWriter)

    [[nodiscard]] virtual bool Process(BufferRef<float> const &input, AudioBuffer<int8_t>& buffer) = 0;
	
    [[nodiscard]] virtual bool Process(float const * samples, size_t num_sample, AudioBuffer<int8_t>& buffer) = 0;

protected:
    ISampleWriter() = default;
};

}
