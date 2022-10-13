//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <stream/ifileencoder.h>

namespace xamp::stream {

enum AACProfileLevel {
	AAC_PROFILE_L2		= 0x29,
	AAC_PROFILE_L4		= 0x2A,
	AAC_PROFILE_L5		= 0x2B,
	AAC_HEV1_PROFILE_L2 = 0x2C,
	AAC_HEV1_PROFILE_L4 = 0x2E,
	AAC_HEV1_PROFILE_L5 = 0x2F,
	AAC_HEV2_PROFILE_L2 = 0x30,
	AAC_HEV2_PROFILE_L3 = 0x31,
	AAC_HEV2_PROFILE_L4 = 0x32,
	AAC_HEV2_PROFILE_L5 = 0x33,
};

enum AACPayloadType {
	PAYLOAD_AAC_RAW = 0,
	PAYLOAD_AAC_ADTS
};

class MFAACFileEncoder final
	: public IFileEncoder {
public:
	MFAACFileEncoder();

	XAMP_PIMPL(MFAACFileEncoder)

	void Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

private:
	class MFAACFileEncoderImpl;
	AlignPtr<MFAACFileEncoderImpl> impl_;
};

}