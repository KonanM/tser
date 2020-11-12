// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
#pragma once
#include <type_traits>
namespace tser {
    template<typename T>
    size_t encode_varint(T value, char* output) {
        size_t i = 0;
        if constexpr (std::is_signed_v<T>)
            value = static_cast<T>(value << 1 ^ (value >> (sizeof(T) * 8 - 1)));
        for (; value > 127; ++i, value >>= 7)
            output[i] = static_cast<char>(static_cast<uint8_t>(value & 127) | 128);
        output[i++] = static_cast<uint8_t>(value) & 127;
        return i;
    }
    template<typename T>
    size_t decode_varint(T& value, const char* const input) {
        size_t i = 0;
        for (value = 0; i == 0 || (input[i - 1] & 128); i++)
            value |= static_cast<T>(input[i] & 127) << (7 * i);
        if constexpr (std::is_signed_v<T>)
            value = (value & 1) ? -static_cast<T>((value + 1) >> 1) : (value + 1) >> 1;
        return i;
    }
}
