#ifndef INCLUDE_GENERATOR_GENERATOR_HPP
#define INCLUDE_GENERATOR_GENERATOR_HPP

#include <cstddef>
#include <filesystem>

namespace elf {

class Generator {
  public:
    struct Config {
        static constexpr std::size_t kPageSize = 0x1000;
        static constexpr std::size_t kDefaultEntryPoint = 4 * kPageSize;

        std::filesystem::path file_name;
        std::size_t entry_point;
        std::size_t segments_count;
        std::size_t segment_size;
    };

    virtual ~Generator() = default;

    virtual void generate(const Config &config) const = 0;

    static constexpr std::size_t align_up(std::size_t value, std::size_t alignment) {
        return ((value / alignment) + (value % alignment != 0)) * alignment;
    }
};

} // namespace elf

#endif // INCLUDE_GENERATOR_GENERATOR_HPP
