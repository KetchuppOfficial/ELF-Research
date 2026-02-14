#ifndef INCLUDE_BENCHMARK_LIBELF_HPP
#define INCLUDE_BENCHMARK_LIBELF_HPP

#include "benchmark/benchmark.hpp"

namespace elf {

class LibELFBenchmark final : public Benchmark {
public:
    virtual Result run(const fs::path &path) const override;
};

} // namespace

#endif // INCLUDE_BENCHMARK_LIBELF_HPP
