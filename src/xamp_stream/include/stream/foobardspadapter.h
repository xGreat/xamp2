//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

#include <base/align_ptr.h>

#include <vector>

namespace xamp::stream {

class XAMP_STREAM_API FoobarDspAdapter final : public IAudioProcessor {
public:
	constexpr static auto Id = std::string_view("8CC030E8-9C41-4D37-88A8-376D0D0ADAE9");

	FoobarDspAdapter();

	XAMP_PIMPL(FoobarDspAdapter)

	void Start(const AnyMap& config) override;

	void Init(const AnyMap& config) override;

	std::vector<std::string> GetAvailableDSPs() const;

	void ConfigPopup(const std::string &name, uint64_t hwnd);

	bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) override;

	uint32_t Process(float const* samples, float* out, uint32_t num_samples) override;

	[[nodiscard]] Uuid GetTypeId() const override;

	[[nodiscard]] std::string_view GetDescription() const noexcept override;

	void Flush() override;

	void AddDSPChain(const std::string& name);

	void RemoveDSPChain(const std::string& name);

	void CloneDSPChainConfig(const FoobarDspAdapter& adapter);

	bool IsActive() const;
private:
	class FoobarDspAdapterImpl;
	AlignPtr<FoobarDspAdapterImpl> impl_;
};

}
