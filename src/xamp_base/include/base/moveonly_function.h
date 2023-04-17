//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <algorithm>
#include <future>
#include <base/base.h>
#include <base/align_ptr.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* MoveOnlyFunction is a function wrapper that can only be moved.
* It is used to wrap a function that is only called once.
*/
class MoveOnlyFunction final {
public:
    template <typename Func>
    MoveOnlyFunction(Func&& f)
        : impl_(MakeAlign<ImplBase, ImplType<Func>>(std::forward<Func>(f))) {
    }

    XAMP_ALWAYS_INLINE void operator()() {
	    impl_->Invoke();
        impl_.reset();
    }

    MoveOnlyFunction() = default;
	
    MoveOnlyFunction(MoveOnlyFunction&& other) noexcept
		: impl_(std::move(other.impl_)) {	    
    }
	
    MoveOnlyFunction& operator=(MoveOnlyFunction&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }
	
    XAMP_DISABLE_COPY(MoveOnlyFunction)
	
private:
    /*
    * ImplBase is a virtual base class for ImplType.
    * It is used to avoid the need to know the type of the function
    * when calling Invoke().
    */
    struct XAMP_NO_VTABLE ImplBase {
        virtual ~ImplBase() = default;
        virtual void Invoke() = 0;
    };

    AlignPtr<ImplBase> impl_;

    template <typename Func>
    struct ImplType final : ImplBase {
	    ImplType(Func&& f)
            : f_(std::forward<Func>(f)) {
        }

        XAMP_ALWAYS_INLINE void Invoke() override {
            std::forward<Func>(f_)();
        }
        Func f_;
    };
};

XAMP_BASE_NAMESPACE_END

