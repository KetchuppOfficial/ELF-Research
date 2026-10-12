#include <algorithm>
#include <stdexcept>

#include <llvm/ADT/StringRef.h>
#include <llvm/Object/ELF.h>

#include <benchmark/benchmark.h>

#include "loader/llvm.hpp"

#include "utils/mmap_wrapper.hpp"

namespace elf {

void LLVMLoader::run(const fs::path &path) const {
    const utils::ReadOnlyFileMapper raw_file{path};

    auto elf = llvm::object::ELF64LEFile::create(
        llvm::StringRef(reinterpret_cast<const char *>(raw_file.data()), raw_file.size()));
    if (auto error = elf.takeError()) {
        throw std::runtime_error{
            std::format("failed to parse ELF file: {}", toString(std::move(error)))};
    }

    auto access_last_byte = [](auto content) {
        if (!content.empty()) {
            char c = content.back();
            benchmark::DoNotOptimize(c);
        }
    };

    auto maybe_segments = elf->program_headers();
    if (auto error = maybe_segments.takeError()) {
        throw std::runtime_error{
            std::format("failed to get segments: {}", toString(std::move(error)))};
    }

    std::ranges::for_each(*maybe_segments | std::views::transform([&](const auto &seg) {
        return *elf->getSegmentContents(seg);
    }),
                          access_last_byte);

    auto maybe_sections = elf->sections();
    if (auto error = maybe_sections.takeError()) {
        throw std::runtime_error{
            std::format("failed to get sections: {}", toString(std::move(error)))};
    }

    std::ranges::for_each(*maybe_sections | std::views::transform([&](const auto &sec) {
        return *elf->getSectionContents(sec);
    }),
                          access_last_byte);
}

} // namespace elf
