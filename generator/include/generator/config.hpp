#ifndef GENERATOR_INCLUDE_GENERATOR_CONFIG_HPP
#define GENERATOR_INCLUDE_GENERATOR_CONFIG_HPP

#include <cstddef>
#include <string_view>
#include <vector>

namespace elf {

class Config final {
  public:
    // Represents either section or segment
    struct Region {
        std::size_t addr;
        std::size_t size;
    };

    static constexpr std::string_view kSchema = R"(
{
    "type": "object",
    "required": ["entry_point", "sections", "segments"],
    "properties": {
        "entry_point": {
            "type": "integer",
            "exclusiveMinimum": 0
        },
        "sections": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["addr", "size"],
                "properties": {
                    "addr": {
                        "type": "integer",
                        "exclusiveMinimum": 0
                    },
                    "size": {
                        "type": "integer",
                        "exclusiveMinimum": 0
                    }
                },
                "additionalProperties": false
            }
        },
        "segments": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["addr", "size"],
                "properties": {
                    "addr": {
                        "type": "integer",
                        "exclusiveMinimum": 0
                    },
                    "size": {
                        "type": "integer",
                        "exclusiveMinimum": 0
                    }
                },
                "additionalProperties": false
            }
        }
    },
    "additionalProperties": false
})";

    explicit Config(std::string_view json);

    std::size_t entry_point() const noexcept { return entry_point_; }

    const auto &sections() const & noexcept { return sections_; }
    const auto &segments() const & noexcept { return segments_; }

  private:
    std::size_t entry_point_;
    std::vector<Region> sections_;
    std::vector<Region> segments_;
};

} // namespace elf

#endif // GENERATOR_INCLUDE_GENERATOR_CONFIG_HPP
