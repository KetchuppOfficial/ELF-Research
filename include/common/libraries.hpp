#ifndef INCLUDE_COMMON_LIBRARIES_HPP
#define INCLUDE_COMMON_LIBRARIES_HPP

#include <meta>
#include <ostream>

#include "utils/enum_support.hpp"

namespace elf {

enum class Libs { kELFIO, kLIEF, kLibELF, kLLVM };

// Needed for pretty help message by CLI11
inline std::ostream &operator<<(std::ostream &os, Libs lib) {
    return os << utils::enum_to_str<Libs, [](std::meta::info e) consteval {
               return utils::identifier_of_google_enumerator(e);
           }>(lib);
}

} // namespace elf

#endif // INCLUDE_COMMON_LIBRARIES_HPP
