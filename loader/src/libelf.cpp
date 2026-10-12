#include <algorithm>

#include <benchmark/benchmark.h>

#include "libelf++/elf.hpp"

#include "loader/libelf.hpp"

namespace elf {

void LibELFLoader::run(const fs::path &path) const {
    using enum utils::FDWrapper::Flags;
    const auto elf =
        use_mmap_ ? elf::ELF{elf::ELF::UseMMap{}, path, kReadOnly} : elf::ELF{path, kReadOnly};

    auto access_last_byte = [](auto content) {
        if (!content.empty()) {
            char c = content.back();
            benchmark::DoNotOptimize(c);
        }
    };

    std::ranges::for_each(elf.segments() |
                              std::views::transform([](elf::Segment seg) { return seg.data(); }),
                          access_last_byte);

    std::ranges::for_each(elf.sections() |
                              std::views::transform([](elf::Section sec) { return sec.data(); }),
                          access_last_byte);
}

} // namespace elf
