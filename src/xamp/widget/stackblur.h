//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <QImage>

#include <base/threadpool.h>

using xamp::base::IThreadPool;

class Stackblur final {
public:
	Stackblur(IThreadPool& tp, QImage& image, uint32_t radius);

private:
	void blur(IThreadPool& tp, uint8_t* src, uint32_t width, uint32_t height, uint32_t radius, uint32_t thread_branch);
};