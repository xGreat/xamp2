//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <string>
#include <ostream>
#include <array>

#include <base/base.h>
#include <base/exception.h>

namespace xamp::base {
	
inline constexpr size_t kIdSize = 16;
inline constexpr size_t kMaxIdStrLen = 36;
using UuidBuffer = std::array<uint8_t, kIdSize>;

class XAMP_BASE_API Uuid final {
public:
    static Uuid const kNullUuid;

    static Uuid FromString(std::string const & str);

    static bool TryParseString(std::string const& str, Uuid &uuid);

    template <typename ForwardIterator>
    explicit Uuid(ForwardIterator first, ForwardIterator last) {
	    if (std::distance(first, last) == kIdSize) {
		    std::copy(first, last, std::begin(bytes_));
	    } else {
            throw std::invalid_argument("Invalid Uuid.");
	    }
    }

    Uuid() noexcept;

    explicit Uuid(UuidBuffer const &bytes) noexcept;

    Uuid(std::string_view const &from_string);

    explicit Uuid(const uint8_t(&arr)[kIdSize]) noexcept;

	[[nodiscard]] bool IsValid() const noexcept;

    Uuid(Uuid const &other) noexcept;
    
    Uuid& operator=(Uuid const &other) noexcept;

    Uuid(Uuid &&other) noexcept;
    
    Uuid& operator=(Uuid &&other) noexcept;

	[[nodiscard]] size_t GetHash() const noexcept;

	operator std::string() const;

    const UuidBuffer & GetBytes() const noexcept {
        return bytes_;
    }

private:
    XAMP_BASE_API friend std::ostream &operator<<(std::ostream &s, Uuid const &id);

    XAMP_BASE_API friend bool operator==(std::string const &str, Uuid const &id);

    XAMP_BASE_API friend bool operator==(Uuid const & other1, Uuid const & other2) noexcept;

    XAMP_BASE_API friend bool operator!=(Uuid const & other1, Uuid const & other2) noexcept;

    UuidBuffer bytes_{0};
};

XAMP_ALWAYS_INLINE bool Uuid::IsValid() const noexcept {
    return bytes_[0] ||
        bytes_[1] ||
        bytes_[2] ||
        bytes_[3] ||
        bytes_[4] ||
        bytes_[5] ||
        bytes_[6] ||
        bytes_[7] ||
        bytes_[8] ||
        bytes_[9] ||
        bytes_[10] ||
        bytes_[11] ||
        bytes_[12] ||
        bytes_[13] ||
        bytes_[14] ||
        bytes_[15];
}

XAMP_ALWAYS_INLINE bool operator!=(Uuid const & other1, Uuid const & other2) noexcept {
    return !(std::memcmp(other1.bytes_.data(), other2.bytes_.data(), other2.bytes_.size()) == 0);
}

XAMP_ALWAYS_INLINE bool operator==(Uuid const & other1, Uuid const & other2) noexcept {
    return std::memcmp(other1.bytes_.data(), other2.bytes_.data(), other2.bytes_.size()) == 0;
}

XAMP_ALWAYS_INLINE bool operator==(std::string const &str, Uuid const &id) {
	return Uuid(str) == id;
}

XAMP_ALWAYS_INLINE size_t Uuid::GetHash() const noexcept {
    const std::string s = *this;
    return std::hash<std::string>{}(s);
}

}

namespace std {

template <>
struct hash<xamp::base::Uuid> {
	typedef size_t result_type;
	typedef xamp::base::Uuid argument_type;

	result_type operator()(argument_type const& uuid) const noexcept {
		return uuid.GetHash();
	}
};

template <>
struct less<xamp::base::Uuid> {
    typedef bool result_type;
    typedef xamp::base::Uuid argument_type;

    result_type operator()(argument_type const& other1, argument_type const& other2) const noexcept {
        return std::memcmp(other1.GetBytes().data(), other2.GetBytes().data(), other2.GetBytes().size()) < 0;
    }
};

}

