#pragma once
#include "serialize.hpp"
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
    template<typename T, std::enable_if_t<(tser::is_detected_v<tser::has_optional_t, T> && !tser::is_detected_v<tser::has_element_t, T>) && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
    std::ostream& operator <<(std::ostream& os, const T& t) {
        if (t) return os << '{' << *t << '}';
        else   return os << '{' << "null" << '}';
    }
}
