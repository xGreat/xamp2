//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/uuid.h>
#include <base/enum.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

XAMP_MAKE_ENUM(DeviceConnectType, UKNOWN, USB, BUILT_IN, BLUE_TOOTH)

struct XAMP_OUTPUT_DEVICE_API DeviceInfo final {
	bool is_default_device{ false };
	bool is_support_dsd{ false };
	bool is_hardware_control_volume{ false };
	float min_volume{ 0 };
	float max_volume{ 0 };
	float volume_increment{ 1.0 };
	DeviceConnectType connect_type{ DeviceConnectType::UKNOWN };
	std::wstring name;    
	std::string device_id;
	Uuid device_type_id;
};

}
