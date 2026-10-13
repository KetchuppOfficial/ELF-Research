#include <cstdint>
#include <ranges>
#include <span>

#include "generator/libelf.hpp"
#include "libelf++/elf.hpp"
#include "utils/range_map.hpp"

namespace elf {

void LibELFGenerator::generate(const std::filesystem::path &file_path) const {
    using enum utils::FDWrapper::Flags;

    constexpr auto kFlags = kWriteOnly | kCreate | kTruncate;
    constexpr auto kPerms = fs::perms::owner_all | fs::perms::others_read | fs::perms::group_read;
    auto elf = use_mmap_ ? elf::ELF{elf::ELF::UseMMap{}, file_path, kFlags, kPerms}
                         : elf::ELF{file_path, kFlags, kPerms};

    auto header = elf.get_header();
    header.set_data_encoding(elf::Header::DataEncoding::kLittleEndian);
    header.set_os_abi(elf::Header::OSABI::kSystemV);
    header.set_type(elf::Header::Type::kExec);
    header.set_machine(elf::Header::Machine::kX86_64);
    header.set_entry(config().entry_point());

    utils::RangeMap<const ::Elf64_Shdr *> section_addr_range_to_section;

    auto data_offset = header.program_header_table_offset() +
                       header.program_header_size() * config().segments().size();

    for (auto [section_info, content] :
         std::views::zip(config().sections(), sections_content_.data())) {
        ::Elf_Scn *section = ::elf_newscn(elf.get());

        ::Elf64_Shdr *sec_header = ::elf64_getshdr(section);
        sec_header->sh_type = SHT_PROGBITS;
        sec_header->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        sec_header->sh_addr = section_info.addr;
        sec_header->sh_addralign = 0x10;
        sec_header->sh_offset = data_offset;

        elf.add_data(section, content, ::Elf_Type::ELF_T_BYTE);

        data_offset += content.size();

        section_addr_range_to_section.emplace(
            utils::InclusiveRange{section_info.addr, section_info.size}, sec_header);
    }

    auto segments = [&] {
        const auto segements_count = config().segments().size();
        ::Elf64_Phdr *segments = ::elf64_newphdr(elf.get(), segements_count);
        return std::span{segments, segements_count};
    }();

    for (auto [segment, segment_info] : std::views::zip(segments, config().segments())) {
        segment.p_type = PT_LOAD;
        segment.p_flags = PF_R | PF_X;
        segment.p_paddr = segment_info.addr;
        segment.p_vaddr = segment_info.addr;
        segment.p_filesz = segment_info.size;
        segment.p_memsz = segment_info.size;
        segment.p_align = 16;

        segment.p_offset = [&] -> ::Elf64_Off {
            auto [sec_begin, sec_end] =
                section_addr_range_to_section.equal_range({segment_info.addr, segment_info.size});
            if (sec_begin == sec_end) {
                return 0;
            } else {
                const auto first_sec_offset = sec_begin->second->sh_offset;
                const std::int64_t first_sec_addr = sec_begin->first.start;
                const std::int64_t segment_addr = segment_info.addr;
                return first_sec_offset + (segment_addr - first_sec_addr);
            }
        }();
    }

    elf.save();
}

} // namespace elf
