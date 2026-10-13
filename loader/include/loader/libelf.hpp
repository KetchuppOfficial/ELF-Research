#ifndef LOADER_INCLUDE_LOADER_LIBELF_HPP
#define LOADER_INCLUDE_LOADER_LIBELF_HPP

#include "loader/loader.hpp"

namespace elf {

class LibELFLoader final : public Loader {
public:
    explicit LibELFLoader(bool use_mmap) : use_mmap_{use_mmap} {}

    virtual void run(const fs::path &path) const override;

private:
    bool use_mmap_;
};

} // namespace

#endif // LOADER_INCLUDE_LOADER_LIBELF_HPP
