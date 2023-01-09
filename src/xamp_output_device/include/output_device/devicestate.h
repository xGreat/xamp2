//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>

namespace xamp::output_device {

XAMP_MAKE_ENUM(DeviceState,
          DEVICE_STATE_ADDED,
          DEVICE_STATE_REMOVED,
          DEVICE_STATE_DEFAULT_DEVICE_CHANGE)

}
