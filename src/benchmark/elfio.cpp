#include <elfio/elfio.hpp>

#include "benchmark/elfio.hpp"

namespace elf {

Benchmark::Result ELFIOBenchmark::run(const fs::path &path) const {
    ELFIO::elfio elf;

    auto load_time = [&] {
        auto start = std::chrono::high_resolution_clock::now();
        elf.load(path);
        auto finish = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<ms>(finish - start);
    }();

    const auto copy = path_to_copy(path);

    auto save_time = [&] {
        auto start = std::chrono::high_resolution_clock::now();
        elf.save(copy);
        auto finish = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<ms>(finish - start);
    }();

    return {load_time, save_time};
}

} // namespace elf
