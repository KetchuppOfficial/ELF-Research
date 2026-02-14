#ifndef INCLUDE_GENERATOR_LIBELF_HPP
#define INCLUDE_GENERATOR_LIBELF_HPP

#include "generator/generator.hpp"

namespace elf {

class LibELFGenerator final : public Generator {
public:
    void generate(const Config &config) const override;
};

} // namespace elf

#endif // INCLUDE_GENERATOR_LIBELF_HPP
