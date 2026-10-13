#ifndef GENERATOR_INCLUDE_GENERATOR_LIBELF_HPP
#define GENERATOR_INCLUDE_GENERATOR_LIBELF_HPP

#include "generator/generator.hpp"
#include "generator/sections_content.hpp"

#include "libelf++/data.hpp"

namespace elf {

class LibELFGenerator final : public Generator {
  public:
    explicit LibELFGenerator(const Config &config, bool use_mmap)
        : Generator{config}, sections_content_(config), use_mmap_{use_mmap} {}

    ~LibELFGenerator() override = default;

    void generate(const std::filesystem::path &file_path) const override;

  private:
    SectionsContent<elf::Data> sections_content_;
    bool use_mmap_;
};

} // namespace elf

#endif // GENERATOR_INCLUDE_GENERATOR_LIBELF_HPP
