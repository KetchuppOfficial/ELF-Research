#ifndef INCLUDE_LOADER_LIEF_HPP
#define INCLUDE_LOADER_LIEF_HPP

#include "loader/loader.hpp"

namespace elf {

class LIEFLoader final : public Loader {
public:
    virtual void run(const fs::path &path) const override;
};

} // namespace

#endif // INCLUDE_LOADER_LIEF_HPP
