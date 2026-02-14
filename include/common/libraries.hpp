#ifndef INCLUDE_COMMON_LIBRARIES_HPP
#define INCLUDE_COMMON_LIBRARIES_HPP

#include <ostream>
#include <string_view>
#include <utility>

namespace elf {

enum class Libs { kELFIO, kLIEF, kLibELF };

namespace {

std::string_view to_string_view(Libs lib) {
    using enum Libs;
    using namespace std::string_view_literals;
    switch (lib) {
    case kELFIO:
        return "elfio"sv;
    case kLIEF:
        return "lief"sv;
    case kLibELF:
        return "libelf"sv;
    default:
        std::unreachable();
    }
}

} // unnamed namespace

// Needed for pretty help message by CLI11
inline std::ostream &operator<<(std::ostream &os, Libs lib) { return os << to_string_view(lib); }

} // namespace elf

#endif // INCLUDE_COMMON_LIBRARIES_HPP
