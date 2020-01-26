// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
#pragma once
// #include "Serialize.hpp"
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <type_traits>

namespace tser
{
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

    template<class T> using enable_for_container_t = std::enable_if_t<is_container_v<T> && !is_trivial_v<T>, int>;
    template<class T> using enable_for_array_t       = std::enable_if_t<is_container_v<T> && !is_trivial_v<T>, int>;
    template<class T> using enable_for_map_t       = std::enable_if_t<std::is_same_v<typename T::value_type::second_type, typename T::mapped_type>, int>;
    template<class T> using enable_for_members_t   = std::enable_if_t<is_detected_v<has_members_t, T> && !is_trivial_v<T>, int>;
    template<class T> using enable_for_smart_ptr_t = std::enable_if_t<is_detected_v<has_element_t, T>, int>;
    template<class T> using enable_for_optional_t  = std::enable_if_t<is_detected_v<has_optional_t, T> && !is_trivial_v<T>, int>;
    template<class T> using enable_for_tuple_t     = std::enable_if_t<is_detected_v<has_tuple_t, T> && !is_trivial_v<T>, int>;
    template<class T> using enable_for_memcpy_t    = std::enable_if_t<is_trivial_v<T>, int>;

    class BinaryArchive {
    public:
        explicit BinaryArchive(const size_t initialSize = 1024) : m_bytes(initialSize, '\0') {}
        //serialization for vector like and set like containers
        template <template <typename...> class TContainer, typename T, enable_for_container_t<TContainer<T>> = 0>
        void save(const TContainer<T>& container) {
            save(static_cast<unsigned short>(container.size()));
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
            save(static_cast<unsigned short>(mapT.size()));
            for (auto& [key, value] : mapT) {
                save(key);
                save(value);
            }
        }
        //serialization for shared_ptr and uniuqe_ptr and the like
        template<typename T, enable_for_smart_ptr_t<T> = 0>
        void save(const T& ptr) {
            save(static_cast<bool>(ptr));
            if (ptr)
                save(*ptr);
        }
        //serialization for optional like types
        template<typename T, enable_for_optional_t<T> = 0>
        void save(const T& opt) {
            save(static_cast<bool>(opt));
            if (opt)
                save(*opt);
        }
        //everything that we can memcopy directly (e.g. array's that are trivially copyable)
        template<typename T, enable_for_memcpy_t<T> = 0>
        void save(const T& t, const unsigned bytes = sizeof(T)) {
            if (m_bufferSize + bytes > m_bytes.size())
                m_bytes.resize((m_bufferSize + bytes) * 2);
            std::memcpy(m_bytes.data() + m_bufferSize, std::addressof(t), bytes);
            m_bufferSize += bytes;
        }
        //iterate over the members of our user defined types 
        template<typename T, enable_for_members_t<T> = 0>
        void save(const T& t) {
            std::apply([&](auto&& ... memberVal) { (save(memberVal), ...); }, t.members());
        }
        //stuff like std::vector and std::string, std::set
        template <template <typename...> class TContainer, typename T, enable_for_container_t<TContainer<T>> = 0>
        void load(TContainer<T>& container) {
            const unsigned short size = load<unsigned short>();
            container.clear();
            for (int i = 0; i < size; ++i) {
                container.insert(container.end(), load<T>());
            }
        }
        //containers like std::array or other multi dim fixed size
        template<template<typename, size_t> class TArray, typename T, size_t N, enable_for_array_t<TArray<T, N>> = 0>
        void load(TArray<T, N>& container) {
            for (auto& val : container)
                load(val);
        }
        // tuple like things like pair or tuple
        template<typename T, enable_for_tuple_t<T> = 0>
        void load(T& tuple) {
            std::apply([&](auto&& ... tupleVal) {(load(tupleVal), ...); }, tuple);
        }
        //stuff like std::map with key value types
        template <template <typename...> class TMap, class TKey, class TVal, enable_for_map_t<TMap<TKey, TVal>> = 0>
        void load(TMap<TKey, TVal>& mapT) {
            mapT.clear();
            const unsigned short size = load<unsigned short>();
            for (int i = 0; i < size; ++i) {
                mapT[load<TKey>()] = load<TVal>();
            }
        }
        //unique_ptr and shared ptr
        template<template <typename...> class TPtr, typename T, enable_for_smart_ptr_t<TPtr<T>> = 0>
        void load(TPtr<T>& ptr) {
            if (const bool isValid = load<bool>()) {
                ptr = TPtr<T>(new T());
                load(*ptr);
            }
            else {
                ptr = nullptr;
            }
        }
        //specialization for optional like types
        template<template <typename...> class TPtr, typename T, enable_for_optional_t<TPtr<T>> = 0>
        void load(TPtr<T>& t) {
            t = load<bool>() ? TPtr<T>(load<T>()) : TPtr<T>();
        }
        //everything that is trivially copyable we just memcpy into the types
        template<typename T, enable_for_memcpy_t<T> = 0>
        void load(T& t) {
            std::memcpy(&t, m_bytes.data() + m_readOffset, sizeof(T));
            m_readOffset += sizeof(T);
        }
        //convenience function to load certain types
        template<typename T>
        T load() {
            T t;
            load(t);
            return t;
        }
        //we iterate through the members of all the types that implement the serizeable macro 
        template<typename T, enable_for_members_t<T> = 0>
        void load(T& t) {
            std::apply([&](auto&& ... memberVal) { (load(memberVal), ...); }, t.members());
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
    //why exactly is there no split function in the STL?
    std::vector<std::string_view> split(std::string_view str, std::string_view delims) {
        std::vector<std::string_view> output;
        for (auto first = str.data(), second = str.data(), last = first + str.size(); second != last && first != last; first = second + 1) {
            second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));
            if (first != second)
                output.emplace_back(first, second - first);
        }
        return output;
    }
}

//this macro defines printing, serialisation and comparision operators (==,!=,<) for custom types
#define DEFINE_SERIALIZABLE(Type, ...)                              \
constexpr inline decltype(auto) members() const { return std::tie(__VA_ARGS__); }    \
constexpr inline decltype(auto) members() { return std::tie(__VA_ARGS__); }    \
static inline const std::vector<std::string_view> _memberNames = tser::split(#__VA_ARGS__, ",");\
static inline const std::string_view _typeName = #Type;

// #include "Compare.hpp"
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

// #include "Serialize.hpp"


//here we can define comparision methods for smart pointer like types (shared_ptr, unique_ptr) to behave like std::optional comparisions
//otherwise the address will be compared instead of the wrapped type
#define DEFINE_SMART_POINTER_COMPARISIONS(Type)\
inline bool operator==(const Type& lhs, const Type& rhs){ if (lhs && rhs) {return *lhs == *rhs;}else if (!lhs && !rhs) {return true;} return false;}\
inline bool operator!=(const Type& lhs, const Type& rhs){ return !(lhs == rhs);}\
inline bool operator< (const Type& lhs, const Type& rhs){ if (lhs && rhs){return *lhs < *rhs;} else if (rhs && !lhs){return true; }; return false;}

//if a complex type doesn't have a hash function and your too lazy to implement one, you could use this ugly hack
#define DEFINE_HASHABLE(Type) \
namespace std { \
        template<> \
        struct hash<Type> { \
            size_t operator()(const Type& t) const { \
                static tser::BinaryArchive bs; \
                bs.reset(); bs.save(t); \
                return std::hash<std::string_view>()(bs.getString()); \
        } \
    }; \
}

template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> && !tser::is_detected_v<tser::has_equal_t  , T>, int> = 0>
inline bool operator==(const T& lhs, const T& rhs) { return lhs.members() == rhs.members(); };
template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> && !tser::is_detected_v<tser::has_nequal_t , T>, int> = 0>
inline bool operator!=(const T& lhs, const T& rhs) { return lhs.members() != rhs.members(); };
template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> && !tser::is_detected_v<tser::has_smaller_t, T>, int> = 0>
inline bool operator< (const T& lhs, const T& rhs) { return lhs.members() < rhs.members(); };

//overload for all containers that are not < comparable already (e.g. std::unordered_map)
template<typename T, typename = std::enable_if_t<tser::is_container_v<T> && !tser::is_detected_v<tser::has_smaller_t, T>>>
bool operator <(const T& lhs, const T& rhs) {
    if (lhs.size() != rhs.size())
        return lhs.size() < rhs.size();
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
//we also need to be able to compare enums
template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
bool operator <(const T& lhs, const T& rhs) {
    using ET = const std::underlying_type_t<T>;
    return static_cast<ET>(lhs) < static_cast<ET>(rhs);
}

// #include "Print.hpp"
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT

// #include "Serialize.hpp"

#include <iostream>

//overload for pair, which is needed to print maps (and pairs)
template<typename X, typename Y>
std::ostream& operator <<(std::ostream& os, const std::pair<X, Y>& p) {
    return os << '{' << p.first << ',' << p.second << '}';
}
template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
std::ostream& operator<<(std::ostream& os, const T& t) {
    int i = -1, last = static_cast<int>(T::_memberNames.size()) - 1;
    os << T::_typeName << ":{ ";
    return std::apply([&](auto&& ... memberVal) -> decltype(auto) {return ((++i, os << T::_memberNames[static_cast<unsigned>(i)] << "=" << memberVal << (i == last ? "}" : ",")), ...); }, t.members());
}
//overload for all containers that don't implement std::ostream& <<
template<typename T, typename = std::enable_if_t<tser::is_container_v<T> && !tser::is_detected_v<tser::has_outstream_op_t, T>>>
std::ostream& operator <<(std::ostream& os, const T& container) {
    os << "[";
    size_t i = 0;
    for (auto& elem : container)
        os << elem << (container.size() == ++i ? ", " : "]\n");
    return os;
}
//enum print support for enums
template<typename T, std::enable_if_t<std::is_enum_v<T> && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
std::ostream& operator <<(std::ostream& os, const T& t) {
    return os << static_cast<std::underlying_type_t<T>>(t);
}

template<typename T, tser::enable_for_optional_t<T> = 0>
std::ostream& operator <<(std::ostream& os, const T& opt) {
    if (opt)
        return os << '{' << *opt << '}';
    else
        return os << '{' << "nullopt" << '}';
}

// #include "Base64Encoding.hpp"

// #include "Serialize.hpp"


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
    std::ostream& operator<<(std::ostream& os, const tser::BinaryArchive& ba)
    {
        //we have to encode to a printable character set only, that's why we use base64
        os << base64_encode(ba.getString()) << '\n';
        return os;
    }
    
    tser::BinaryArchive& operator<<(tser::BinaryArchive& ba, std::string encoded)
    {
        ba.initialize(base64_decode(encoded));
        return ba;
    }
}

