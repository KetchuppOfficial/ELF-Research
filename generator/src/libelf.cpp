#include <cstdint>
#include <map>
#include <ranges>
#include <span>

#include "generator/libelf.hpp"

#include "libelf++/elf.hpp"

namespace elf {

void LibELFGenerator::generate(const std::filesystem::path &file_path) const {
    struct InclusiveRange {
        std::uint64_t start;
        std::uint64_t size;
    };

    struct Compare {
        bool operator()(const InclusiveRange &lhs, const InclusiveRange &rhs) const noexcept {
            return lhs.start + lhs.size <= rhs.start;
        }
    };

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

    std::map<InclusiveRange, const ::Elf64_Shdr *, Compare> section_addr_range_to_section;

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

        section_addr_range_to_section.emplace(InclusiveRange{section_info.addr, section_info.size},
                                              sec_header);
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

        auto *section = section_addr_range_to_section.at({segment_info.addr, 1});
        segment.p_offset = section->sh_offset + (segment_info.addr - section->sh_addr);
    }

    elf.save();
}

} // namespace elf
