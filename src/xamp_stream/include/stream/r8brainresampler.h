//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

static constexpr int32_t kR8brainBufferSize = 64 * 1024;

class XAMP_STREAM_API R8brainSampleRateConverter final : public IAudioProcessor {
public:
    static const std::string_view VERSION;

    XAMP_PIMPL(R8brainSampleRateConverter)

	R8brainSampleRateConverter();

    void Start(const DspConfig& config) override;

    void Init(const DspConfig& config) override;

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;

private:
    class R8brainSampleRateConverterImpl;
    AlignPtr<R8brainSampleRateConverterImpl> impl_;
};
XAMP_MAKE_CLASS_UUID(R8brainSampleRateConverter, "786D706E-20F0-4F30-9B98-8B489DC5C739")

}

