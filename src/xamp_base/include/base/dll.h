//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/fs.h>
#include <base/assert.h>
#include <base/platfrom_handle.h>

#include <type_traits>
#include <string_view>

namespace xamp::base {

XAMP_BASE_API SharedLibraryHandle LoadSharedLibrary(const std::string_view& file_name);

XAMP_BASE_API SharedLibraryHandle OpenSharedLibrary(const std::string_view& file_name);

XAMP_BASE_API Path GetSharedLibraryPath(const SharedLibraryHandle &module);

XAMP_BASE_API void* LoadSharedLibrarySymbol(const SharedLibraryHandle& dll, const std::string_view & name);

XAMP_BASE_API bool PrefetchSharedLibrary(SharedLibraryHandle const& module);

XAMP_BASE_API bool AddSharedLibrarySearchDirectory(const Path &path);

#ifdef XAMP_OS_WIN
XAMP_BASE_API void* LoadSharedLibrarySymbolEx(SharedLibraryHandle const& dll, const std::string_view name, uint32_t flags);
XAMP_BASE_API SharedLibraryHandle PinSystemLibrary(const std::string_view& file_name);
#endif

template
<
    typename T,
    typename U = std::enable_if_t<std::is_function_v<T>>
>
class XAMP_BASE_API_ONLY_EXPORT SharedLibraryFunction final {
public:
    SharedLibraryFunction(SharedLibraryHandle const& dll, const std::string_view name) {
        func_ = static_cast<T*>(LoadSharedLibrarySymbol(dll, name));
    }

#ifdef XAMP_OS_WIN
    SharedLibraryFunction(SharedLibraryHandle const& dll, const std::string_view name, uint32_t flags) {
        func_ = static_cast<T*>(LoadSharedLibrarySymbolEx(dll, name, flags));
    }
#endif

    [[nodiscard]] XAMP_ALWAYS_INLINE operator T* () const noexcept {
        XAMP_ASSERT(func_ != nullptr);
        return func_;
    }

    [[nodiscard]] XAMP_ALWAYS_INLINE bool IsValid() const noexcept {
        return func_ != nullptr;
    }

    XAMP_DISABLE_COPY_AND_MOVE(SharedLibraryFunction)
private:
    T *func_{nullptr};
};

#define XAMP_DECLARE_DLL(Func) SharedLibraryFunction<decltype(Func)>
#define XAMP_DECLARE_DLL_NAME(Func) SharedLibraryFunction<decltype(Func)> Func
#define XAMP_LOAD_DLL_API(Func) Func(module_, #Func)
#define XAMP_LOAD_DLL_API_EX(MemberName, Func) MemberName(module_, #Func)

}
