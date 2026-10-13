#ifndef GENERATOR_INCLUDE_GENERATOR_ELFIO_HPP
#define GENERATOR_INCLUDE_GENERATOR_ELFIO_HPP

#include <vector>

#include "generator/generator.hpp"
#include "generator/sections_content.hpp"

namespace elf {

class ELFIOGenerator final : public Generator {
  public:
    ELFIOGenerator(const Config &config) : Generator{config}, sections_content_{config} {}

    ~ELFIOGenerator() override = default;

    void generate(const std::filesystem::path &file_path) const override;

  private:
    SectionsContent<std::vector<char>> sections_content_;
};

} // namespace elf

#endif // GENERATOR_INCLUDE_GENERATOR_ELFIO_HPP
