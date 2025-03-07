//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <atomic>
#include <memory>
#include <new>

XAMP_BASE_NAMESPACE_BEGIN

// Harald Achitz: Launder lazy_storage
// https://www.youtube.com/watch?v=743l-mSqtsI
template <typename T>
class XAMP_BASE_API_ONLY_EXPORT LocalStorage {
public:
	constexpr LocalStorage() noexcept = default;

	LocalStorage(LocalStorage&& other) noexcept {
		other.data_ = std::move(data_);
		other.need_init_ = true;
		other.has_data_ = false;
	}

	LocalStorage& operator=(LocalStorage&&) = delete;

	~LocalStorage() {
		reset();
	}

	void reset() {
		if (has_data_.load()) {
			std::destroy_at(reinterpret_cast<T*>(std::addressof(data_)));
			has_data_ = false;
		}
	}

	XAMP_DISABLE_COPY(LocalStorage)

	constexpr T* get() {
		wait_for_init_done();
		return std::launder(reinterpret_cast<T*>(&data_));
	}

	constexpr const T* get() const {
		wait_for_init_done();
		return std::launder(reinterpret_cast<T*>(&data_));
	}

	const T * operator->() const {
		return get();
	}

	T* operator->() {
		return get();
	}
	
private:
	void wait_for_init_done() {
		const auto do_need_init = need_init_.exchange(false);
		if (do_need_init) {
			std::construct_at(reinterpret_cast<T*>(std::addressof(data_)));
			has_data_ = true;
		}
		else {
			while (!has_data_.load()) {}
		}
	}

	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<bool> need_init_{ true };
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<bool> has_data_{ false };
	alignas(T) std::byte data_[sizeof(T)]{};
};

XAMP_BASE_NAMESPACE_END


