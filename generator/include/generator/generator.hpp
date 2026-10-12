#ifndef INCLUDE_GENERATOR_GENERATOR_HPP
#define INCLUDE_GENERATOR_GENERATOR_HPP

#include <cstddef>
#include <filesystem>
#include <functional>

#include "generator/config.hpp"

namespace elf {

class Generator {
  public:
    static constexpr std::size_t kPageSize = 0x1000;
    static constexpr std::size_t kDefaultAlignment = kPageSize;
    static constexpr std::size_t kDefaultEntryPoint = 4 * kPageSize;

    explicit Generator(const Config &config) : config_{config} {}

    virtual ~Generator() = default;

    virtual void generate(const std::filesystem::path &file_path) const = 0;

    static constexpr std::size_t align_up(std::size_t value, std::size_t alignment) noexcept {
        return ((value / alignment) + (value % alignment != 0)) * alignment;
    }

protected:
    const Config &config() const noexcept { return config_.get(); }

private:
    std::reference_wrapper<const Config> config_;
};

} // namespace elf

#endif // INCLUDE_GENERATOR_GENERATOR_HPP
