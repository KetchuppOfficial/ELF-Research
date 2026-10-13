#ifndef LOADER_INCLUDE_LOADER_LIEF_HPP
#define LOADER_INCLUDE_LOADER_LIEF_HPP

#include "loader/loader.hpp"

namespace elf {

class LIEFLoader final : public Loader {
public:
    virtual void run(const fs::path &path) const override;
};

} // namespace

#endif // LOADER_INCLUDE_LOADER_LIEF_HPP
