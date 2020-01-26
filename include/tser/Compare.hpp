// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
#pragma once
#include "Serialize.hpp"

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
