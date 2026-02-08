#ifndef INCLUDE_BENCHMARK_LIEF_HPP
#define INCLUDE_BENCHMARK_LIEF_HPP

#include "benchmark/benchmark.hpp"

namespace elf {

class LIEFBenchmark final : public Benchmark {
public:
    virtual Result run(const fs::path &path) const override;
};

} // namespace

#endif // INCLUDE_BENCHMARK_LIEF_HPP
