#ifndef INCLUDE_GENERATOR_ELFIO_HPP
#define INCLUDE_GENERATOR_ELFIO_HPP

#include "generator/generator.hpp"

namespace elf {

class ELFIOGenerator final : public Generator {
public:
    void generate(const Config &config) const override;
};

} // namespace elf

#endif // INCLUDE_GENERATOR_ELFIO_HPP
