#include <algorithm>

#include <elfio/elfio.hpp>

#include <benchmark/benchmark.h>

#include "loader/elfio.hpp"

namespace elf {

void ELFIOLoader::run(const fs::path &path) const {
    ELFIO::elfio elf;
    elf.load(path);

    std::ranges::for_each(elf.segments, [](const auto &seg) {
        if (seg->get_file_size() != 0) {
            auto c = seg->get_data()[seg->get_file_size() - 1];
            benchmark::DoNotOptimize(c);
        }
    });

    std::ranges::for_each(elf.sections, [](const auto &sec){
        if (sec->get_size() != 0) {
            auto c = sec->get_data()[sec->get_size() - 1];
            benchmark::DoNotOptimize(c);
        }
    });
}

} // namespace elf
