//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>

namespace xamp::base {

XAMP_MAKE_ENUM(DsdFormat,
          DSD_INT8LSB,
          DSD_INT8MSB,
          DSD_INT8NER8)

XAMP_MAKE_ENUM(DsdModes,
          DSD_MODE_AUTO,
          DSD_MODE_PCM,
          DSD_MODE_NATIVE,
          DSD_MODE_DOP,
		  DSD_MODE_DOP_AA,
		  DSD_MODE_DSD2PCM)

}
