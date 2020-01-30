#pragma once
#include "serialize.hpp"

//implement comparision operators for tser classes that don't implement the operators themselves
template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> && !tser::is_detected_v<tser::has_equal_t  , T>, int> = 0>
inline bool operator==(const T& lhs, const T& rhs) { return lhs.members() == rhs.members(); };
template<typename T, std::enable_if_t<tser::is_detected_v<tser::has_members_t, T> && !tser::is_detected_v<tser::has_nequal_t , T>, int> = 0>
inline bool operator!=(const T& lhs, const T& rhs) { return !(lhs == rhs); };
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
