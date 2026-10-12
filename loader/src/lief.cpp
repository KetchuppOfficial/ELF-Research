#include <algorithm>

#include <LIEF/LIEF.hpp>

#include <benchmark/benchmark.h>

#include "loader/lief.hpp"

namespace elf {

void LIEFLoader::run(const fs::path &path) const {
    const auto elf = LIEF::ELF::Parser::parse(path);

    auto access_last_byte = [](const auto &region) {
        const auto content = region.content();
        if (!content.empty()) {
            char c = content.back();
            benchmark::DoNotOptimize(c);
        }
    };

    auto segments = elf->segments();
    std::for_each(segments.begin(), segments.end(), access_last_byte);

    auto sections = elf->sections();
    std::for_each(sections.begin(), sections.end(), access_last_byte);
}

} // namespace elf
