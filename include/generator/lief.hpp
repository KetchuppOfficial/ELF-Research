#ifndef INCLUDE_GENERATOR_LIEF_HPP
#define INCLUDE_GENERATOR_LIEF_HPP

#include "generator/generator.hpp"

namespace elf {

class LIEFGenerator final : public Generator {
public:
    void generate(const Config &config) const override;
};

} // namespace elf

#endif // INCLUDE_GENERATOR_LIEF_HPP
