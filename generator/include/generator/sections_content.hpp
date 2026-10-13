#ifndef GENERATOR_INCLUDE_GENERATOR_SECTIONS_CONTENT_HPP
#define GENERATOR_INCLUDE_GENERATOR_SECTIONS_CONTENT_HPP

#include <algorithm>
#include <random>
#include <vector>

#include "generator/config.hpp"

namespace elf {

template <typename C>
class SectionsContent final {
  public:
    explicit SectionsContent(const Config &config) {
        std::default_random_engine engine;
        data_.reserve(config.sections().size());
        for (const auto &region : config.sections()) {
            auto &data = data_.emplace_back(region.size);
            std::ranges::generate(data, std::ref(engine));
        }
    }

    auto &data() & noexcept { return data_; }
    const auto &data() const & noexcept { return data_; }

  private:
    std::vector<C> data_;
};

} // namespace elf

#endif // GENERATOR_INCLUDE_GENERATOR_SECTIONS_CONTENT_HPP
