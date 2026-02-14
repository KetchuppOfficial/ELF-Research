#include <ranges>

#include <elfio/elfio.hpp>

#include "generator/elfio.hpp"

namespace elf {

void ELFIOGenerator::generate(const Config &config) const {
    ELFIO::elfio elf;
    elf.create(ELFIO::ELFCLASS64, ELFIO::ELFDATA2LSB);
    elf.set_type(ELFIO::ET_EXEC);
    elf.set_machine(ELFIO::EM_X86_64);
    elf.set_entry(Config::kDefaultEntryPoint);

    const auto size = config.segment_size;
    std::size_t addr = Config::kDefaultEntryPoint;
    std::vector<char> data(size, 'Z');

    for (const auto _ : std::views::iota(0uz, config.segments_count)) {
        const auto *section = [&] {
            auto *section = elf.sections.add("");
            section->set_type(ELFIO::SHT_PROGBITS);
            section->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_EXECINSTR);
            section->set_addr_align(0x10);
            section->set_data(data.data(), data.size());
            return section;
        }();

        auto *segment = [&] {
            auto *segment = elf.segments.add();
            segment->set_type(ELFIO::PT_LOAD);
            segment->set_flags(ELFIO::PF_R | ELFIO::PF_X);
            segment->set_physical_address(addr);
            segment->set_virtual_address(addr);
            segment->set_file_size(size);
            segment->set_memory_size(size);
            segment->set_align(Config::kPageSize);
            return segment;
        }();

        segment->add_section_index(section->get_index(), section->get_addr_align());
        addr = align_up(addr + size, Config::kPageSize);
    }

    elf.save(config.file_name.native());
}

} // namespace elf
