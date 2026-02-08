#include <memory>
#include <utility>

#include <LIEF/LIEF.hpp>

#include "benchmark/lief.hpp"

namespace elf {

Benchmark::Result LIEFBenchmark::run(const fs::path &path) const {
    const auto [elf, load_time] = [&] -> std::pair<std::unique_ptr<LIEF::ELF::Binary>, ms> {
        auto start = std::chrono::high_resolution_clock::now();
        auto elf = LIEF::ELF::Parser::parse(path);
        auto finish = std::chrono::high_resolution_clock::now();
        return {std::move(elf), std::chrono::duration_cast<ms>(finish - start)};
    }();

    auto save_time = [&] {
        auto start = std::chrono::high_resolution_clock::now();
        elf->write(path_to_copy(path));
        auto finish = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<ms>(finish - start);
    }();

    return {load_time, save_time};
}

} // namespace elf
