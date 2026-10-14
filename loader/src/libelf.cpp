#include <algorithm>

#include <benchmark/benchmark.h>

#include "libelf++/elf.hpp"

#include "loader/libelf.hpp"

namespace elf {

void LibELFLoader::run(const fs::path &path) const {
    using enum utils::FDWrapper::Flags;
    const auto elf =
        use_mmap_ ? elf::ELF{elf::ELF::UseMMap{}, path, kReadOnly} : elf::ELF{path, kReadOnly};

    auto access_last_bytes = []<std::ranges::range R>(R &&range) {
        std::ranges::for_each(
            std::forward<R>(range),
            [](auto content) {
                if (!content.empty()) {
                    char c = content.back();
                    benchmark::DoNotOptimize(c);
                }
            },
            [](auto region) { return region.data(); });
    };

    access_last_bytes(elf.segments());
    access_last_bytes(elf.sections());
}

} // namespace elf
