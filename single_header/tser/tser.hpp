// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
#pragma once
// #include "serialize.hpp"// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#include <array>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>

namespace tser
{
    template<typename T>
    size_t encodeVarInt(T value, char* output) {
        size_t i = 0;
        if constexpr (std::is_signed_v<T>)
            value = value << 1 ^ (value >> (sizeof(T) * 8 - 1));
        for (; value > 127; ++i, value >>= 7)
            output[i] = static_cast<uint8_t>(value & 127) | 128;
        output[i++] = static_cast<uint8_t>(value) & 127;
        return i;
    }
    template<typename T>
    size_t decodeVarInt(T& value, char* input) {
        size_t i = 0;
        for (value = 0; i == 0 || (input[i - 1] & 128); i++)
            value |= static_cast<T>(input[i] & 127) << (7 * i);
        if constexpr (std::is_signed_v<T>)
            value = (value & 1) ? -static_cast<T>((value + 1) >> 1) : (value + 1) >> 1;
        return i;
    }
    //implementation details for is_detected
    namespace detail {
        struct ns {
            ~ns() = delete;
            ns(ns const&) = delete;
        };
        template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
        struct detector {
            using value_t = std::false_type;
            using type = Default;
        };
        template <class Default, template<class...> class Op, class... Args>
        struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };
    } // namespace detail

    // we need a bunch of template metaprogramming for being able to differentiate between different types 
    template <template<class...> class Op, class... Args>
    constexpr bool is_detected_v = detail::detector<detail::ns, void, Op, Args...>::value_t::value;

    template<class T> using has_begin_t = decltype(*std::begin(std::declval<T>()));
    template<class T> using has_members_t = decltype(std::declval<T>().members());
    template<class T> using has_smaller_t = decltype(std::declval<T>() < std::declval<T>());
    template<class T> using has_equal_t = decltype(std::declval<T>() == std::declval<T>());
    template<class T> using has_nequal_t = decltype(std::declval<T>() != std::declval<T>());
    template<class T> using has_outstream_op_t = decltype(std::declval<std::ostream>() << std::declval<T>());
    template<class T> using has_tuple_t = typename std::tuple_element<0, T>::type;
    template<class T> using has_optional_t = decltype(std::declval<T>().has_value());
    template<class T> using has_element_t = typename T::element_type;
    template<class T> constexpr bool is_container_v = is_detected_v<has_begin_t, T>;
    template<class T> constexpr bool is_trivial_v = std::is_trivially_copyable_v<T>;
    template<class T> constexpr bool is_pointer_v = std::is_pointer_v<T> || tser::is_detected_v<has_element_t, T>;

    class BinaryArchive {
    public:
        template<class T> using has_custom_save_t = decltype(std::declval<T>().save(std::declval<BinaryArchive&>()));
        template<class T> using enable_for_container_t = std::enable_if_t<is_container_v<T> && !is_trivial_v<T>, int>;
        template<class T> using enable_for_array_t = std::enable_if_t<is_container_v<T>, int>;
        template<class T> using enable_for_map_t = std::enable_if_t<std::is_same_v<typename T::value_type::second_type, typename T::mapped_type>, int>;
        template<class T> using enable_for_members_t = std::enable_if_t<is_detected_v<has_members_t, T> && !is_detected_v<has_custom_save_t, T>, int>;
        template<class T> using enable_for_smart_ptr_t = std::enable_if_t<is_detected_v<has_element_t, T>, int>;
        template<class T> using enable_for_optional_t = std::enable_if_t<is_detected_v<has_optional_t, T>, int>;
        template<class T> using enable_for_pointer_t = std::enable_if_t<std::is_pointer_v<std::remove_cv_t<std::remove_reference_t<T>>>, int>;
        template<class T> using enable_for_tuple_t = std::enable_if_t<is_detected_v<has_tuple_t, T> && !is_trivial_v<T>, int>;
        template<class T> using enable_for_custom_t = std::enable_if_t<is_detected_v<has_custom_save_t, T>, int>;
        template<class T> using enable_for_memcpy_t = std::enable_if_t<is_trivial_v<T> && !std::is_pointer_v<T> && !is_detected_v<has_members_t, T> && !is_detected_v<has_optional_t, T> && !is_detected_v<has_custom_save_t, T>, int>;

        explicit BinaryArchive(const size_t initialSize = 1024) : m_bytes(initialSize, '\0') {}
        //serialization for vector like and set like containers
        template <template <typename...> class TContainer, typename T, enable_for_container_t<TContainer<T>> = 0>
        void save(const TContainer<T>& container) {
            save(std::size(container));
            for (auto& val : container)
                save(val);
        }
        //serialization for array like (fixed size) containers
        template<template<typename, size_t> class TArray, typename T, size_t N, enable_for_array_t<TArray<T, N>> = 0>
        void save(const TArray<T, N>& container) {
            for (auto& val : container)
                save(val);
        }
        //serialization for std::tuple and std::pair
        template<typename T, enable_for_tuple_t<T> = 0>
        void save(const T& tuple) {
            std::apply([&](auto&& ... tupleVal) {(save(tupleVal), ...); }, tuple);
        }
        //serialization for map like containers with key value pairs
        template <typename T, enable_for_map_t<T> = 0>
        void save(const T& mapT) {
            save(mapT.size());
            for (auto& [key, value] : mapT) {
                save(key);
                save(value);
            }
        }
        //serialization for shared_ptr and uniuqe_ptr and the like
        template<typename T, enable_for_smart_ptr_t<T> = 0>
        void save(const T& ptr) {
            savePtrLike(ptr);
        }
        //serialization for optional like types
        template<typename T, enable_for_optional_t<T> = 0>
        void save(const T& opt) {
            savePtrLike(opt);
        }
        template<typename T, int N>
        void save(T(&carray)[N])
        {
            for (auto& val : carray)
                save(val);
        }
        template<typename T, enable_for_pointer_t<T> = 0>
        void save(T&& ptr) {
            savePtrLike(ptr);
        }
        template<typename T>
        void savePtrLike(T&& ptr)
        {
            save(static_cast<bool>(ptr));
            if (ptr)
                save(*ptr);
        }
        //iterate over the members of our user defined types 
        template<typename T, enable_for_members_t<T> = 0>
        void save(const T& t) {
            std::apply([&](auto&& ... memberVal) { (save(memberVal), ...); }, t.members());
        }
        //here we invoke a custom save method for a type that provides this function
        template<typename T, enable_for_custom_t<T> = 0>
        void save(const T& t) {
            t.save(*this);
        }
        //everything that we can memcopy directly (e.g. array's that are trivially copyable)
        template<typename T, enable_for_memcpy_t<T> = 0>
        void save(const T& t, const unsigned bytes = sizeof(T)) {
            if (m_bufferSize + bytes > m_bytes.size())
                m_bytes.resize((m_bufferSize + bytes) * 2);
            if constexpr (!std::is_floating_point_v<T> && !std::is_enum_v<T> && sizeof(T) > 2)
                m_bufferSize += encodeVarInt(t, m_bytes.data() + m_bufferSize);
            else{
                std::memcpy(m_bytes.data() + m_bufferSize, std::addressof(t), bytes);
                m_bufferSize += bytes;
            }
        }
        //small helper so that we can use operator << to save any type
        template<typename T>
        friend BinaryArchive& operator&(BinaryArchive& ba, T&& t) {
            ba.save(std::forward<T>(t)); return ba;
        }
        //stuff like std::vector and std::string, std::set
        template <template <typename...> class TContainer, typename T, enable_for_container_t<TContainer<T>> = 0>
        void load(TContainer<T>& container) {
            const auto size = load<decltype(container.size())>();
            container.clear();
            for (int i = 0; i < size; ++i)
                container.insert(container.end(), load<T>());
        }
        //containers like std::array or other multi dim fixed size
        template<template<typename, size_t> class TArray, typename T, size_t N, enable_for_array_t<TArray<T, N>> = 0>
        void load(TArray<T, N>& container) {
            for (auto& val : container)
                load(val);
        }
        // tuple like things (pair or tuple)
        template<typename T, enable_for_tuple_t<T> = 0>
        void load(T& tuple) {
            std::apply([&](auto&& ... tupleVal) {(load(tupleVal), ...); }, tuple);
        }
        //stuff like std::map with key value types
        template <template <typename...> class TMap, class TKey, class TVal, enable_for_map_t<TMap<TKey, TVal>> = 0>
        void load(TMap<TKey, TVal>& mapT) {
            mapT.clear();
            const auto size = load<decltype(mapT.size())>();
            for (int i = 0; i < size; ++i) {
                auto key = load<TKey>();
                mapT[std::move(key)] = load<TVal>();
            }
        }
        //unique_ptr and shared ptr
        template<template <typename...> class TPtr, typename T, enable_for_smart_ptr_t<TPtr<T>> = 0>
        void load(TPtr<T>& ptr) {
            ptr = TPtr<T>(load<T*>());
        }
        //specialization for optional like types
        template<template <typename...> class TPtr, typename T, enable_for_optional_t<TPtr<T>> = 0>
        void load(TPtr<T>& t) {
            t = load<bool>() ? TPtr<T>(load<T>()) : TPtr<T>();
        }
        //raw pointers
        template<typename T, enable_for_pointer_t<T> = 0>
        void load(T& t) {
            delete t;
            t = load<T>();
        }
        template<typename T, int N>
        void load(T(&carray)[N])
        {
            for (auto& val : carray)
                load(val);
        }
        //we iterate through the members of all the types that implement the serializeable macro 
        template<typename T, enable_for_members_t<T> = 0>
        void load(T& t) {
            std::apply([&](auto&& ... memberVal) { (load(memberVal), ...); }, t.members());
        }
        //here we invoke a custom load method for a type that provides this function
        template<typename T, enable_for_custom_t<T> = 0>
        void load(T& t) {
            t.load(*this);
        }
        //everything that is trivially copyable we just memcpy into the types
        template<typename T, enable_for_memcpy_t<T> = 0>
        void load(T& t) {
            if constexpr (!std::is_floating_point_v<T> && !std::is_enum_v <T> && sizeof(T) > 2)
                m_readOffset += decodeVarInt(t, m_bytes.data() + m_readOffset);
            else{
                std::memcpy(&t, m_bytes.data() + m_readOffset, sizeof(T));
                m_readOffset += sizeof(T);
            }
        }
        //convenience function to load a specified type
        template<typename T>
        T load() {
            T t{};
            if constexpr (std::is_pointer_v<T>){
                if (load<bool>()){
                    t = new std::remove_pointer_t<T>{};
                    load(*t);
                }
            }
            else {
                load(t);
            }
            return t;
        }
        void reset() {
            m_bufferSize = 0;
            m_readOffset = 0;
        }
        void initialize(std::string_view str) {
            if (str.size() > m_bytes.size())
                m_bytes.resize(str.size() + 1);
            m_bytes = str;
            m_bufferSize = str.size();
            m_readOffset = 0;
        }
        std::string_view getString() const {
            return std::string_view(m_bytes.data(), m_bufferSize);
        }
    private:
        std::string m_bytes = std::string(1024, '\0');
        //current bit offset
        size_t m_bufferSize = 0, m_readOffset = 0;
    };

    constexpr size_t nArgs(std::string_view str) {
        size_t nargs = 1; for (auto c : str) if (c == ',') ++nargs; return nargs;
    }
}

//this macro defines printing, serialisation and comparision operators (==,!=,<) for custom types
#define DEFINE_SERIALIZABLE(Type, ...) \
constexpr inline decltype(auto) members() const { return std::tie(__VA_ARGS__); } \
constexpr inline decltype(auto) members() { return std::tie(__VA_ARGS__); }  \
static constexpr std::string_view _typeName = #Type;\
static constexpr std::array<std::string_view, tser::nArgs(#__VA_ARGS__)> _memberNames = [](){ std::array<std::string_view, tser::nArgs(#__VA_ARGS__)> out{}; \
size_t off = 0, next = 0; std::string_view ini(#__VA_ARGS__); \
for(auto& strV : out){ next = ini.find_first_of(',', off); strV = ini.substr(off, next - off); off = next + 1;} return out;}();\
template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> &&  !tser::is_detected_v<tser::has_equal_t, T>, int> = 0>\
friend bool operator==(const Type& lhs, const T& rhs) { return lhs.members() == rhs.members(); }\
template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> &&  !tser::is_detected_v<tser::has_nequal_t, T>, int> = 0>\
friend bool operator!=(const Type& lhs, const T& rhs) { return !(lhs == rhs); }\
template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> &&  !tser::is_detected_v<tser::has_smaller_t, T>, int> = 0>\
friend bool operator< (const Type& lhs, const T& rhs) { return lhs.members() < rhs.members(); }


// #include "compare.hpp"
// #include "serialize.hpp"

//implement comparision operators for serializable classes that don't implement the operators themselves
namespace std {
    //overload for all containers that are not < comparable already (e.g. std::unordered_map)
    template<typename T, typename = std::enable_if_t<tser::is_container_v<T> && !tser::is_detected_v<tser::has_smaller_t, T>>>
    inline bool operator <(const T& lhs, const T& rhs) {
        if (lhs.size() != rhs.size())
            return lhs.size() < rhs.size();
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    //we also need to be able to compare enums
    template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
    inline bool operator <(const T& lhs, const T& rhs) {
        using ET = const std::underlying_type_t<T>;
        return static_cast<ET>(lhs) < static_cast<ET>(rhs);
    }
}

// #include "print.hpp"
// #include "serialize.hpp"
#include <iostream>
namespace std
{
    //overload for pair, which is needed to print maps (and pairs)
    template<typename X, typename Y>
    std::ostream& operator <<(std::ostream& os, const std::pair<X, Y>& p) {
        return os << '{' << p.first << ',' << p.second << '}';
    }
    //overload for classes that implement the tser macro
    template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
    std::ostream& operator<<(std::ostream& os, const T& t) {
        int i = -1, last = static_cast<int>(T::_memberNames.size()) - 1;
        os << T::_typeName << ":{";
        return std::apply([&](auto&& ... memberVal) -> decltype(auto) {return ((++i, os << T::_memberNames[static_cast<unsigned>(i)] << "=" << memberVal << (i == last ? "}" : ",")), ...); }, t.members());
    }
    //overload for all containers that don't implement std::ostream& <<
    template<typename T, typename = std::enable_if_t<tser::is_container_v<T> && !tser::is_detected_v<tser::has_outstream_op_t, T>>>
    std::ostream& operator <<(std::ostream& os, const T& container) {
        os << "[";
        size_t i = 0;
        for (auto& elem : container)
            os << elem << ((++i) != container.size() ? ", " : "]\n");
        return os;
    }
    //print support for enums
    template<typename T, std::enable_if_t<std::is_enum_v<T> && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
    std::ostream& operator <<(std::ostream& os, const T& t) {
        return os << static_cast<std::underlying_type_t<T>>(t);
    }
    //support for optional like types
    template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_optional_t, T> && !tser::is_detected_v<tser::has_element_t, T> && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
    std::ostream& operator <<(std::ostream& os, const T& t) {
        if (t) return os << '{' << *t << '}';
        else   return os << '{' << "null" << '}';
    }
}

// #include "base64encoding.hpp"
// #include "serialize.hpp"

namespace tser
{
    //tables for the base64 conversions
    static constexpr auto g_encodingTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static constexpr auto g_decodingTable = []() { std::array<char, 256> decodingTable{}; for (char i = 0; i < 64; ++i) decodingTable[static_cast<unsigned>(g_encodingTable[static_cast<size_t>(i)])] = i; return decodingTable; }();

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
        return out;
    }

    static std::string base64_decode(std::string_view in) {
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

