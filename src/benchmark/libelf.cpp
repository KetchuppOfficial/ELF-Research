#include <stdexcept>

#include <fcntl.h>
#include <unistd.h>

#include <gelf.h>
#include <libelf.h>

#include "benchmark/libelf.hpp"
#include "utils/fd_wrapper.hpp"
#include "libelf++/elf.hpp"

namespace elf {

Benchmark::Result LibELFBenchmark::run(const fs::path &path) const {
    using enum utils::FDWrapper::Flags;

    auto start = std::chrono::high_resolution_clock::now();
    elf::ELF elf{path, kReadOnly};
    auto elf_span = elf.raw_contents();
    auto finish = std::chrono::high_resolution_clock::now();
    auto load_time = std::chrono::duration_cast<ms>(finish - start);

    start = std::chrono::high_resolution_clock::now();
    utils::FDWrapper copy{path_to_copy(path), kWriteOnly | kCreate,
                          fs::perms::owner_all | fs::perms::others_read | fs::perms::group_read};
    const auto wrote =
        utils::write_all(copy, reinterpret_cast<unsigned char *>(elf_span.data()), elf_span.size());
    if (wrote != elf_span.size()) {
        throw std::runtime_error(std::format("wrote {}/{} bytes", wrote, elf_span.size()));
    }

    finish = std::chrono::high_resolution_clock::now();
    auto save_time = std::chrono::duration_cast<ms>(finish - start);

    return {load_time, save_time};
}

} // namespace elf
