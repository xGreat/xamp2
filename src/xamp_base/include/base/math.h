//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>
#include <complex>
#include <base/base.h>

namespace xamp::base {

static XAMP_ALWAYS_INLINE int32_t NextPowerOfTwo(int32_t v) noexcept {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

static XAMP_ALWAYS_INLINE int32_t IsPowerOfTwo(int32_t v) noexcept {
	return v > 0 && !(v & (v - 1));
}

static XAMP_ALWAYS_INLINE double toMag(const std::complex<float>& r) {
	return 10.0 * std::log10(std::pow(r.real(), 2) + std::pow(r.imag(), 2));
}

template <typename T>
T Round(T a) {
    static_assert(std::is_floating_point_v<T>, "Round<T>: T must be floating point");
    return (a > 0) ? ::floor(a + static_cast<T>(0.5)) : ::ceil(a - static_cast<T>(0.5));
}

template <typename T>
T Round(T a, int32_t places) {
    static_assert(std::is_floating_point_v<T>, "Round<T>: T must be floating point");
    const T shift = pow(static_cast<T>(10.0), places);
    return Round(a * shift) / shift;
}

}

