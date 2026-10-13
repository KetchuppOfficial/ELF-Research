#ifndef LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_HEADER_HPP
#define LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_HEADER_HPP

#include <utility>

#include <libelf.h>

namespace elf {

class Header final {
  public:
    Header(::Elf64_Ehdr &header) : header_{&header} {}

    Elf64_Half size() const noexcept { return header_->e_ehsize; }

    Elf64_Half segments_count() const noexcept { return header_->e_phnum; }
    Elf64_Half program_header_size() const noexcept { return header_->e_phentsize; }
    Elf64_Off program_header_table_offset() const noexcept { return header_->e_phoff; }

    Elf64_Half sections_count() const noexcept { return header_->e_shnum; }
    Elf64_Half section_header_size() const noexcept { return header_->e_shentsize; }
    Elf64_Off section_header_table_offset() const noexcept { return header_->e_shoff; }

    Elf64_Word get_flags() const noexcept { return header_->e_flags; }
    void set_flags(Elf64_Word flags) noexcept { header_->e_flags = flags; }

    Elf64_Addr get_entry() const noexcept { return header_->e_entry; }
    void set_entry(Elf64_Addr entry) noexcept { header_->e_entry = entry; }

    enum class DataEncoding : unsigned char {
        kNone = ELFDATANONE,
        kLittleEndian = ELFDATA2LSB,
        kBigEndian = ELFDATA2MSB
    };

    DataEncoding get_data_encoding() const noexcept {
        return DataEncoding{header_->e_ident[EI_DATA]};
    }
    void set_data_encoding(DataEncoding e) noexcept {
        header_->e_ident[EI_DATA] = std::to_underlying(e);
    }

    enum class OSABI : unsigned char {
        kSystemV = ELFOSABI_SYSV,
    };

    OSABI get_os_abi() const noexcept { return OSABI{header_->e_ident[EI_OSABI]}; }
    void set_os_abi(OSABI abi) noexcept { header_->e_ident[EI_OSABI] = std::to_underlying(abi); }

    enum class Type : Elf64_Half {
        kNone = ET_NONE,
        kRel = ET_REL,
        kExec = ET_EXEC,
        kDynamic = ET_DYN,
        kCore = ET_CORE,
        kOSSpecificStard = ET_LOOS,
        kOSSpecificEnd = ET_HIOS,
        kCPUSpecificStart = ET_LOPROC,
        kCPUSpecificEnd = ET_HIPROC
    };

    Type get_type() const noexcept { return Type{header_->e_type}; }
    void set_type(Type type) noexcept { header_->e_type = std::to_underlying(type); }

    enum class Machine : Elf64_Half { kX86_64 = EM_X86_64 };

    Machine get_machine() const noexcept { return Machine{header_->e_machine}; }
    void set_machine(Machine machine) noexcept { header_->e_machine = std::to_underlying(machine); }

  private:
    ::Elf64_Ehdr *header_;
};

} // namespace elf

#endif // LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_HEADER_HPP
