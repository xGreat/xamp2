//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <base/base.h>
#include <base/math.h>

XAMP_BASE_NAMESPACE_BEGIN

// Ref: https://github.com/google/highwayhash
template <int TUpdateRounds = 2, int TFinalizeRounds = 4>
class XAMP_BASE_API_ONLY_EXPORT GoogleSipHash {
public:
    static constexpr int kPacketSize = sizeof(uint64_t);

    constexpr GoogleSipHash(uint64_t k0 = 0, uint64_t k1 = 0) noexcept {
        v0_ = 0x736f6d6570736575ull ^ k0;
        v1_ = 0x646f72616e646f6dull ^ k1;
        v2_ = 0x6c7967656e657261ull ^ k0;
        v3_ = 0x7465646279746573ull ^ k1;
    }

    void Update(const std::string& x) noexcept {
        Update(x.c_str(), x.length());
    }

    template <typename CharT>
    void Update(const std::basic_string< CharT>& str) noexcept {
        Update(reinterpret_cast<const char*>(str.data()), str.size() * sizeof(CharT));
    }

    template <typename CharT>
    void Update(const CharT *str, size_t length) noexcept {
        Update(reinterpret_cast<const char*>(str), length * sizeof(CharT));
    }

    XAMP_ALWAYS_INLINE void Update(const char* bytes, size_t size) noexcept {
        const size_t remainder = size & (kPacketSize - 1);
        const size_t truncated_size = size - remainder;

        for (size_t i = 0; i < truncated_size; i += kPacketSize) {
            Update(bytes + i);
        }

        PaddedUpdate(size, bytes + truncated_size, remainder);
    }

    XAMP_ALWAYS_INLINE void Finalize() noexcept {
        v2_ ^= 0xFF;
        Compress<TFinalizeRounds>();
    }

    XAMP_ALWAYS_INLINE uint64_t GetHash() noexcept {
        Finalize();
        return v0_ ^ v1_ ^ v2_ ^ v3_;
    }


    static uint64_t GetHash(const std::string& x, uint64_t k0, uint64_t k1) {
        GoogleSipHash hasher(k0, k1);
        hasher.Update(x);
        return hasher.GetHash();
    }

private:
    XAMP_ALWAYS_INLINE void PaddedUpdate(size_t size, const char* remaining_bytes, const uint64_t remaining_size) noexcept {
        char final_packet[kPacketSize] = { 0 };
        MemoryCopy(final_packet, remaining_bytes, remaining_size);
        final_packet[kPacketSize - 1] = static_cast<char>(size & 0xFF);
        Update(final_packet);
    }

    XAMP_ALWAYS_INLINE void Update(const char* bytes) noexcept {
        uint64_t packet;
        MemoryCopy(&packet, bytes, sizeof(packet));
        packet = host_from_le64(packet);
        v3_ ^= packet;
        Compress<TUpdateRounds>();
        v0_ ^= packet;
    }

    template <size_t TRounds>
    void Compress() noexcept {
        for (size_t i = 0; i < TRounds; ++i) {
            v0_ += v1_;
            v2_ += v3_;
            v1_ = Rotl64<13>(v1_);
            v3_ = Rotl64<16>(v3_);
            v1_ ^= v0_;
            v3_ ^= v2_;

            v0_ = Rotl64<32>(v0_);

            v2_ += v1_;
            v0_ += v3_;
            v1_ = Rotl64<17>(v1_);
            v3_ = Rotl64<21>(v3_);
            v1_ ^= v2_;
            v3_ ^= v0_;

            v2_ = Rotl64<32>(v2_);
        }
    }

    uint64_t v0_;
    uint64_t v1_;
    uint64_t v2_;
    uint64_t v3_;
};

XAMP_BASE_NAMESPACE_END
