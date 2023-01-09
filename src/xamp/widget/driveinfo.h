//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <qmetatype.h>
#include <widget/str_utilts.h>

struct DriveInfo {
	char driver_letter;
	QString display_name;
	QString drive_path;
};
Q_DECLARE_METATYPE(DriveInfo)

