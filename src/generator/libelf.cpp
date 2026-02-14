#include <cstddef>
#include <ranges>

#include "generator/libelf.hpp"

#include "libelf++/elf.hpp"

namespace elf {

void LibELFGenerator::generate(const Config &config) const {
    using enum utils::FDWrapper::Flags;
    elf::ELF elf{config.file_name, kWriteOnly | kCreate | kTruncate,
                 fs::perms::owner_all | fs::perms::others_read | fs::perms::group_read};

    auto header = elf.get_header();
    header.set_data_encoding(elf::Header::DataEncoding::kLittleEndian);
    header.set_os_abi(elf::Header::OSABI::kSystemV);
    header.set_type(elf::Header::Type::kExec);
    header.set_machine(elf::Header::Machine::kX86_64);
    header.set_entry(Config::kDefaultEntryPoint);

    const auto size = config.segment_size;

    std::ranges::repeat_view input(static_cast<unsigned char>('Z'), size);

    ::Elf64_Phdr *segments = ::elf64_newphdr(elf.get(), config.segments_count);

    std::size_t addr = Config::kDefaultEntryPoint;
    auto data_offset = header.program_header_table_offset() +
                       header.program_header_size() * header.segments_count();

    elf::Data data{input};

    for (auto &segment : std::views::counted(segments, config.segments_count)) {
        ::Elf_Scn *section = ::elf_newscn(elf.get());
        ::Elf64_Shdr *sec_header = ::elf64_getshdr(section);

        sec_header->sh_type = SHT_PROGBITS;
        sec_header->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        sec_header->sh_addr = addr;
        sec_header->sh_addralign = 0x10;
        sec_header->sh_offset = data_offset;

        segment.p_type = PT_LOAD;
        segment.p_flags = PF_R | PF_X;
        segment.p_paddr = addr;
        segment.p_vaddr = addr;
        segment.p_filesz = size;
        segment.p_memsz = size;
        segment.p_align = Config::kPageSize;
        segment.p_offset = data_offset;

        elf.add_data(section, data, ::Elf_Type::ELF_T_BYTE);

        addr = align_up(addr + size, Config::kPageSize);
        data_offset += size;
    }

    elf.save();
}

} // namespace elf
