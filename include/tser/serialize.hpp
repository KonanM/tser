// Licensed under the Boost License <https://opensource.org/licenses/BSL-1.0>.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>
#include "tser/varint_encoding.hpp"

namespace tser{
    static std::string encode_base64(std::string_view in);
    static std::string decode_base64(std::string_view in);
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
        constexpr size_t n_args(char const* c, size_t nargs = 1) {
            for (; *c; ++c) if (*c == ',') ++nargs; return nargs;
        }
        constexpr size_t str_size(char const* c, size_t strSize = 1) {
            for (; *c; ++c) ++strSize; return strSize;
        }
    }
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
        std::string m_bytes = std::string(1024, '\0');
        size_t m_bufferSize = 0, m_readOffset = 0;
    public:
        explicit BinaryArchive(const size_t initialSize = 1024) : m_bytes(initialSize, '\0') {}
        explicit BinaryArchive(std::string encodedStr) : m_bytes(decode_base64(encodedStr)), m_bufferSize(m_bytes.size()){}

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
        template<typename T, size_t N>
        void save(const T(&carray)[N]) {
            for (auto& val : carray)
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
        template<typename T, enable_for_pointer_t<T> = 0>
        void save(T&& ptr) {
            savePtrLike(ptr);
        }
        template<typename T>
        void savePtrLike(T&& ptr){
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
            if constexpr (std::is_integral_v<T> && sizeof(T) > 2)
                m_bufferSize += encode_varint(t, m_bytes.data() + m_bufferSize);
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
            for (size_t i = 0; i < size; ++i)
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
            for (size_t i = 0; i < size; ++i) {
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
        void load(T(&carray)[N]){
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
            if constexpr (std::is_integral_v<T> && sizeof(T) > 2)
                m_readOffset += decode_varint(t, m_bytes.data() + m_readOffset);
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
            m_bytes = str;
            m_bufferSize = str.size();
            m_readOffset = 0;
        }
        std::string_view get_buffer() const {
            return std::string_view(m_bytes.data(), m_bufferSize);
        }
        friend std::ostream& operator<<(std::ostream& os, const BinaryArchive& ba) {
            return os << encode_base64(ba.get_buffer()) << '\n';
        }
    };

}
//this macro defines printing, serialisation and comparision operators (==,!=,<) for custom types
#define DEFINE_SERIALIZABLE(Type, ...) \
inline decltype(auto) members() const { return std::tie(__VA_ARGS__); } \
inline decltype(auto) members() { return std::tie(__VA_ARGS__); }  \
static constexpr std::array<char, tser::detail::str_size(#__VA_ARGS__)> _memberNameData = [](){ \
std::array<char, tser::detail::str_size(#__VA_ARGS__)> chars{}; size_t idx = 0; constexpr auto* ini(#__VA_ARGS__);  \
for (char const* c = ini; *c; ++c, ++idx) if(*c != ',') chars[idx] = *c;  return chars;}(); \
static constexpr const char* _typeName = #Type; \
static constexpr std::array<const char*, tser::detail::n_args(#__VA_ARGS__)> _memberNames = \
[](){ std::array<const char*, tser::detail::n_args(#__VA_ARGS__)> out{ {Type::_memberNameData.data()} }; \
for(size_t i = 0, nArgs = 0; i < Type::_memberNameData.size() - 1; ++i) \
if (Type::_memberNameData[i] == '\0'){++nArgs; out[nArgs] = &Type::_memberNameData[i] + 1;} \
return out;}();\
template<typename OT, std::enable_if_t<tser::is_detected_v<tser::has_members_t, OT> &&  !tser::is_detected_v<tser::has_equal_t, OT>, int> = 0>\
friend bool operator==(const Type& lhs, const OT& rhs) { return lhs.members() == rhs.members(); }\
template<typename OT, std::enable_if_t<tser::is_detected_v<tser::has_members_t, OT> &&  !tser::is_detected_v<tser::has_nequal_t, OT>, int> = 0>\
friend bool operator!=(const Type& lhs, const OT& rhs) { return !(lhs == rhs); }\
template<typename OT, std::enable_if_t<tser::is_detected_v<tser::has_members_t, OT> &&  !tser::is_detected_v<tser::has_smaller_t, OT>, int> = 0>\
friend bool operator< (const Type& lhs, const OT& rhs) { return lhs.members() < rhs.members(); }
