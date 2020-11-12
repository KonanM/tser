#pragma once
#include "serialize.hpp"
namespace std {
    constexpr inline auto printVal = [](std::ostream& os, auto&& val) -> auto&& { 
        if constexpr (std::is_constructible_v<std::string, decltype(val)> || std::is_same_v<std::decay_t<decltype(val)>, char>)
            return ((os << "\"" << val), "\""); else
        return val;
    };
    //overload for pair, which is needed to print maps (and pairs)
    template<typename X, typename Y>
    std::ostream& operator <<(std::ostream& os, const std::pair<X, Y>& p) {
        return os << "{ \"key\": " << printVal(os, p.first) << ", \"value\": " << printVal(os, p.second) << "}";
    }
    //overload for classes that implement the tser macro
    template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
    std::ostream& operator<<(std::ostream& os, const T& t) {
        int i = -1, last = static_cast<int>(T::_memberNames.size()) - 1;
        auto pMem = [&](auto&& ... memberVal) -> std::ostream& {return ((++i, os << '\"' << T::_memberNames[static_cast<unsigned>(i)] << "\" : " << printVal(os, memberVal) << (i == last ? "}\n" : ", ")), ...); };
        return (os << "{ \"" << T::_typeName << "\": {", std::apply(pMem, t.members())) << "}";
    }
    //overload for all containers that don't implement std::ostream& <<
    template<typename T, typename = std::enable_if_t<tser::is_container_v<T> && !tser::is_detected_v<tser::has_outstream_op_t, T>>>
    std::ostream& operator <<(std::ostream& os, const T& container) {
        os << "\n[ ";
        size_t i = 0;
        for (auto& elem : container)
            os << printVal(os, elem) << ((++i) != container.size() ? "," : "]\n");
        return os;
    }
    //print support for enums
    template<typename T, std::enable_if_t<std::is_enum_v<T> && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
    std::ostream& operator <<(std::ostream& os, const T& t) {
        return os << printVal(os, static_cast<std::underlying_type_t<T>>(t));
    }
    //support for optional like types
    template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_optional_t, T> && !tser::is_detected_v<tser::has_element_t, T> && !tser::is_detected_v<tser::has_outstream_op_t, T>, int> = 0>
    std::ostream& operator <<(std::ostream& os, const T& t) {
        return os << (t ? (os << (printVal(os, *t)), "") : "\"null\"");
    }
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
