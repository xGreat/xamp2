//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <sstream>
#include <string>
#include <iomanip>
#include <algorithm>
#include <charconv>
#include <vector>

#include <spdlog/fmt/fmt.h>
#include <base/base.h>

namespace xamp::base::String {

XAMP_BASE_API std::wstring ToStdWString(std::string const& utf8);

XAMP_BASE_API std::string ToUtf8String(std::wstring const& utf16);

XAMP_ALWAYS_INLINE std::wstring ToString(std::string const& utf8) {
	return ToStdWString(utf8);
}

XAMP_ALWAYS_INLINE std::string ToString(std::wstring const& utf16) {
	return ToUtf8String(utf16);
}

XAMP_ALWAYS_INLINE std::string AsStdString(const std::string_view& s) {
    return { s.data(), s.size() };
}

XAMP_BASE_API bool IsStringsEqual(std::wstring const& str1, std::wstring const& str2);

XAMP_BASE_API std::string LocaleStringToUTF8(const std::string& str);

template <typename CharType>
std::basic_string<CharType> ToUpper(std::basic_string<CharType> s) {
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

template <typename CharType>
std::basic_string<CharType> ToLower(std::basic_string<CharType> s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

template <typename CharType>
void LTrim(std::basic_string<CharType> &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch) {
                return !std::isspace(ch);
            }));
}

template <typename CharType>
void RTrim(std::basic_string<CharType> &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](auto ch) {
                return !std::isspace(ch);
            }).base(), s.end());
}

template <typename CharType>
void Remove(std::basic_string<CharType>& s, const std::basic_string<CharType>& p) {
    auto n = p.length();

    for (auto i = s.find(p);
        i != std::basic_string<CharType>::npos;
        i = s.find(p))
        s.erase(i, n);
}

template <typename CharType>
void Remove(std::basic_string<CharType>& s, const CharType* target) {
    std::basic_string<CharType> p(target);
    Remove(s, p);
}

XAMP_BASE_API std::string FormatBytes(size_t bytes) noexcept;

#if 0
XAMP_ALWAYS_INLINE std::errc ParseDouble(std::string const& s, double &d) {
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), d);
    return ec;
}
#endif

template <typename T>
XAMP_ALWAYS_INLINE std::string FormatBytesBy(size_t bytes) noexcept {
    return FormatBytes(sizeof(T) * bytes);
}

template <typename  C>
std::string Join(C const& pieces, std::string_view const separator = ",") {
    std::string s;
    s.reserve(pieces.size() * 16);
    auto prev = std::prev(pieces.end());
	
    for (auto p = pieces.begin(); p != pieces.end(); ++p) {
        s += *p;
        if (p != prev) {
            s += separator;
        }            
    }
    return s;
}

template <typename T>
XAMP_ALWAYS_INLINE std::vector<std::basic_string_view<T>> Split(std::basic_string_view<T> s,
    const std::basic_string_view<T> delims = " ") {
    std::vector<std::basic_string_view<T>> output;
    size_t first = 0;

    while (first < s.size()) {
        const auto second = s.find_first_of(delims, first);

        if (first != second) {
            output.emplace_back(s.substr(first, second - first));
        }            

        if (second == std::string_view::npos) {
            break;
        }
        first = second + 1;
    }
    return output;
}

template <typename T>
XAMP_ALWAYS_INLINE std::vector<std::basic_string_view<T>> Split(const T* s,
    const T* delims = " ") {
    return Split(std::basic_string_view<T>(s), std::basic_string_view<T>(delims));
}

template <typename... Args>
XAMP_ALWAYS_INLINE std::string Format(std::string_view s, Args &&...args) {
    return fmt::format(s, args...);
}

}
