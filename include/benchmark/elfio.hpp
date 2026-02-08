#ifndef INCLUDE_BENCHMARK_ELFIO_HPP
#define INCLUDE_BENCHMARK_ELFIO_HPP

#include "benchmark/benchmark.hpp"

namespace elf {

class ELFIOBenchmark final : public Benchmark {
public:
    virtual Result run(const fs::path &path) const override;
};

} // namespace

#endif // INCLUDE_BENCHMARK_ELFIO_HPP
