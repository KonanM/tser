#pragma once
#include "serialize.hpp"

namespace tser
{
    //tables for the base64 conversions
    static constexpr auto g_encodingTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static constexpr auto g_decodingTable = []() { std::array<char, 256> decodingTable{ -1 }; for (char i = 0; i < 64; ++i) decodingTable[static_cast<unsigned>(g_encodingTable[static_cast<size_t>(i)])] = i; return decodingTable; }();

    static std::string base64_encode(std::string_view in) {
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
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    static std::string base64_decode(std::string_view in) {
        std::string out;
        int val = 0, valb = -8;
        for (char c : in) {
            if (g_decodingTable[static_cast<unsigned char>(c)] == -1) break;
            val = (val << 6) + g_decodingTable[static_cast<unsigned char>(c)];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }
    //here we define the operators to print to the console and converts that result back into the binary archive
    std::ostream& operator<<(std::ostream& os, const tser::BinaryArchive& ba){
        //we have to encode to a printable character set only, that's why we use base64
        os << base64_encode(ba.getString()) << '\n';
        return os;
    }
    
    tser::BinaryArchive& operator<<(tser::BinaryArchive& ba, std::string encoded){
        ba.initialize(base64_decode(encoded));
        return ba;
    }
}
