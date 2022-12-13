//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/output_device.h>

#include <base/exception.h>
#include <base/fs.h>

namespace xamp::output_device::win32 {

class XAMP_OUTPUT_DEVICE_API HRException final : public PlatformSpecException {
public:
	static void ThrowFromHResult(long hresult, std::string_view expr = "", const Path& file_path = "", int32_t line_number = 0);

	explicit HRException(long hresult, std::string_view expr = "", const Path& file_path = "", int32_t line_number = 0);

	long GetHResult() const;

	const char* GetExpression() const noexcept override;

	std::string GetFileNameAndLine() const;

private:
	long hr_;
	std::string_view expr_;
	std::string file_name_and_line_;
};

}

#define HrIfFailledThrow2(hresult, otherHr) \
	do { \
		if (FAILED((hresult)) && ((otherHr) != (hresult))) { \
			HRException::ThrowFromHResult(hresult, #hresult, __FILE__, __LINE__); \
		} \
	} while (false)

#define HrIfFailledThrow(hresult) \
	do { \
		if (FAILED((hresult))) { \
			HRException::ThrowFromHResult(hresult, #hresult, __FILE__, __LINE__); \
		} \
	} while (false)

#endif
