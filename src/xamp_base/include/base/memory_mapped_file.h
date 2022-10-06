//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/align_ptr.h>
#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API MemoryMappedFile {
public:
    MemoryMappedFile();

    XAMP_PIMPL(MemoryMappedFile)

    void Open(std::wstring const &file_path, bool is_module = false);

    void const * GetData() const noexcept;

    size_t GetLength() const;

	void Close() noexcept;

private:
	class MemoryMappedFileImpl;
	AlignPtr<MemoryMappedFileImpl> impl_;
};

}

