//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class BassEqualizer final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassEqualizer, "FCC73B23-6806-44CD-882D-EA21A3482F51")

public:
    constexpr static auto Description = std::string_view("BassEqualizer");

    BassEqualizer();

    XAMP_PIMPL(BassEqualizer)

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    void SetEq(uint32_t band, float gain, float Q);

    void SetEq(EqSettings const &settings);

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    void SetPreamp(float preamp);

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

private:
    class BassEqualizerImpl;
    AlignPtr<BassEqualizerImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
