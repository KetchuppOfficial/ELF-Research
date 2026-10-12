#include <cstdint>
#include <map>
#include <ranges>
#include <vector>

#include <elfio/elfio.hpp>

#include "generator/elfio.hpp"

namespace elf {

void ELFIOGenerator::generate(const std::filesystem::path &file_path) const {
    struct InclusiveRange {
        std::uint64_t start;
        std::uint64_t size;
    };

    struct Compare {
        bool operator()(const InclusiveRange &lhs, const InclusiveRange &rhs) const noexcept {
            return lhs.start + lhs.size <= rhs.start;
        }
    };

    ELFIO::elfio elf;
    elf.create(ELFIO::ELFCLASS64, ELFIO::ELFDATA2LSB);
    elf.set_type(ELFIO::ET_EXEC);
    elf.set_machine(ELFIO::EM_X86_64);
    elf.set_entry(config().entry_point());

    std::map<InclusiveRange, ELFIO::segment *, Compare> segment_addr_range_to_segment;

    for (const auto &[idx, region] : std::views::enumerate(config().segments())) {
        auto *segment = elf.segments.add();
        segment->set_type(ELFIO::PT_LOAD);
        segment->set_flags(ELFIO::PF_R | ELFIO::PF_X);
        segment->set_physical_address(region.addr);
        segment->set_virtual_address(region.addr);
        segment->set_file_size(region.size);
        segment->set_memory_size(region.size);
        segment->set_align(16);
        segment_addr_range_to_segment.emplace(InclusiveRange{region.addr, region.size}, segment);
    }

    for (const auto &[region, data] :
         std::views::zip(config().sections(), sections_content_.data())) {
        auto *section = elf.sections.add("");
        section->set_type(ELFIO::SHT_PROGBITS);
        section->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_EXECINSTR);
        section->set_address(region.addr);
        section->set_addr_align(16);
        section->set_data(data.data(), data.size());

        auto *seg = segment_addr_range_to_segment.at(InclusiveRange{region.addr, region.size});
        seg->add_section(section, 16);
    }

    elf.save(file_path.native());
}

} // namespace elf
