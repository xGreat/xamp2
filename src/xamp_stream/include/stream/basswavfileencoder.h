//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/ifileencoder.h>

#include <base/align_ptr.h>
#include <base/uuidof.h>

namespace xamp::stream {

class BassWavFileEncoder final 	: public IFileEncoder {
	XAMP_DECLARE_MAKE_CLASS_UUID(BassWavFileEncoder, "41592357-F396-4222-965C-E5A465E128C1")

public:
	BassWavFileEncoder();

	XAMP_PIMPL(BassWavFileEncoder)

	void Start(const AnyMap& config) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

private:
	class BassWavFileEncoderImpl;
	AlignPtr<BassWavFileEncoderImpl> impl_;
};

}
