#ifndef INCLUDE_LOADER_ELFIO_HPP
#define INCLUDE_LOADER_ELFIO_HPP

#include "loader/loader.hpp"

namespace elf {

class ELFIOLoader final : public Loader {
public:
    virtual void run(const fs::path &path) const override;
};

} // namespace

#endif // INCLUDE_BENCHMARK_ELFIO_HPP
