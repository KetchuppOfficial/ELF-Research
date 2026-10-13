#ifndef LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_ERROR_HPP
#define LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_ERROR_HPP

#include <system_error>

#include <libelf.h>

namespace elf {

struct LibELFCategory final : public std::error_category {
    const char *name() const noexcept override { return "libelf error"; }
    std::string message(int errc) const override { return ::elf_errmsg(errc); }
};

inline std::error_category &libelf_category() noexcept {
    static LibELFCategory category;
    return category;
}

} // namespace elf

#endif // LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_ERROR_HPP
