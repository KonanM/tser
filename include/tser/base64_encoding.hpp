// Licensed under the Boost License <https://opensource.org/licenses/BSL-1.0>.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <array>
#include <string>
#include <string_view>
namespace tser {
    //tables for the base64 conversions
    static constexpr auto g_encodingTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static constexpr auto g_decodingTable = []() { std::array<char, 256> decTable{}; for (char i = 0; i < 64; ++i) decTable[static_cast<unsigned>(g_encodingTable[static_cast<size_t>(i)])] = i; return decTable; }();
    static std::string encode_base64(std::string_view in) {
        std::string out;
        int val = 0, valb = -6;
        for (char c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(g_encodingTable[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(g_encodingTable[((val << 8) >> (valb + 8)) & 0x3F]);
        return out;
    }
    static std::string decode_base64(std::string_view in) {
        std::string out;
        int val = 0, valb = -8;
        for (char c : in) {
            val = (val << 6) + g_decodingTable[static_cast<unsigned char>(c)];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }
}
