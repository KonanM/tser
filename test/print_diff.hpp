// Licensed under the Boost License <https://opensource.org/licenses/BSL-1.0>.
// SPDX-License-Identifier: BSL-1.0
#include "tser/tser.hpp"

#include <string>
#include <ostream>
#include <algorithm>


//if you need deep pointer comparisions you could grab this macro
namespace tser {
    template <class T>
    void print_diff(const T& lhs, const T& rhs, std::ostream& cerr, const char* typeName = T::_typeName);

    template <class T, class Tuple, std::size_t... I>
    void print_diffTuple(const Tuple& lhs, const Tuple& rhs, std::ostream& os, std::index_sequence<I...>){
        (print_diff(std::get<I>(lhs), std::get<I>(rhs), os, T::_memberNames[I]), ...);
    }
    template<typename T>
    void print_diff(const T& lhs, const T& rhs, std::ostream& os, const char* typeName){
        if constexpr (tser::is_container_v<T>){
            const bool isEq = std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
            if (!isEq){
                const size_t lastElem = std::min(lhs.size(), rhs.size());

                os << "\n\"" << typeName << "\": \n{";
                for (size_t i = 0; i < lastElem; ++i) {
                    if (lhs[i] != rhs[i]) {
                        std::string idx = "[" + std::to_string(i) + "]:";
                        print_diff(lhs[i], rhs[i], os, idx.c_str());
                        os << '\n';
                    }
                }
                if (lhs.size() > rhs.size()) {
                    os << " Additional element(s) in first type detected! \n";
                    for (size_t i = lastElem; i < lhs.size(); ++i) {
                        os << lhs[i] << '\n';
                    }
                }
                else if (rhs.size() > lhs.size()) {
                    os << " Additional element(s) in second type detected! \n";
                    for (size_t i = lastElem; i < rhs.size(); ++i) {
                        os << rhs[i] << '\n';
                    }
                }
                os << "}\n";
            }
        }
        else if constexpr (tser::is_tser_t_v<T>) {
            if (lhs != rhs)
            {
                os << '\"' << typeName << "\": {";
                print_diffTuple<T>(lhs.members(), rhs.members(), os, std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(lhs.members())>>>());
                os << "}";
            }
        }
        else if constexpr (tser::is_pointer_like_v<T>) {
            if (lhs && rhs)
                print_diff(*lhs, *rhs, os, typeName);
            else if (lhs && !rhs)
                os << "\"" << typeName << "\": (" << lhs << "/ null )";
            else if (rhs && !lhs)
                os << "\"" << typeName << "\": ( null /" << rhs << ")";
        }
        else if (lhs != rhs)
            os << '\"' << typeName << "\": (" << lhs << "/" << rhs << ")";
    }
}
