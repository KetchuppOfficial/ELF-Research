#include <ranges>
#include <vector>

#include <elfio/elfio.hpp>

#include "generator/elfio.hpp"
#include "utils/range_map.hpp"

namespace elf {

void ELFIOGenerator::generate(const std::filesystem::path &file_path) const {
    ELFIO::elfio elf;
    elf.create(ELFIO::ELFCLASS64, ELFIO::ELFDATA2LSB);
    elf.set_type(ELFIO::ET_EXEC);
    elf.set_machine(ELFIO::EM_X86_64);
    elf.set_entry(config().entry_point());

    utils::RangeMap<ELFIO::section *> section_addr_range_to_section;

    for (const auto &[region, data] :
         std::views::zip(config().sections(), sections_content_.data())) {
        auto *section = elf.sections.add("");
        section->set_type(ELFIO::SHT_PROGBITS);
        section->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_EXECINSTR);
        section->set_address(region.addr);
        section->set_addr_align(16);
        section->set_data(data.data(), data.size());

        section_addr_range_to_section.emplace(utils::InclusiveRange{region.addr, region.size},
                                              section);
    }

    for (const auto &[idx, region] : std::views::enumerate(config().segments())) {
        auto *segment = elf.segments.add();
        segment->set_type(ELFIO::PT_LOAD);
        segment->set_flags(ELFIO::PF_R | ELFIO::PF_X);
        segment->set_physical_address(region.addr);
        segment->set_virtual_address(region.addr);
        segment->set_file_size(region.size);
        segment->set_memory_size(region.size);
        segment->set_align(16);

        auto [sec_begin, sec_end] =
            section_addr_range_to_section.equal_range({region.addr, region.size});
        for (; sec_begin != sec_end; ++sec_begin) {
            segment->add_section(sec_begin->second, 16);
        }
    }

    elf.save(file_path.native());
}

} // namespace elf
