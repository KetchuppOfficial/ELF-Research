#ifndef LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_SEGMENT_HPP
#define LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_SEGMENT_HPP

#include <span>
#include <utility>

#include <libelf.h>

namespace elf {

class Segment final {
  public:
    enum Type : ::Elf64_Word { kLoad = PT_LOAD };

    Type get_type() const noexcept { return Type{program_header_->p_type}; }
    void set_type(Type type) noexcept { program_header_->p_type = std::to_underlying(type); }

    ::Elf64_Word get_flags() const noexcept { return program_header_->p_flags; }
    void set_flags(::Elf64_Word flags) noexcept { program_header_->p_flags = flags; }

    ::Elf64_Off get_file_offset() const noexcept { return program_header_->p_offset; }
    ::Elf64_Addr get_phys_addr() const noexcept { return program_header_->p_paddr; }
    ::Elf64_Addr get_virt_addr() const noexcept { return program_header_->p_vaddr; }

    ::Elf64_Xword get_file_size() const noexcept { return program_header_->p_filesz; }
    ::Elf64_Xword get_mem_size() const noexcept { return program_header_->p_memsz; }

    ::Elf64_Xword get_alignment() const noexcept { return program_header_->p_align; }

    std::span<unsigned char> data() { return {elf_raw_data_ + get_file_offset(), get_file_size()}; }
    std::span<const unsigned char> data() const {
        return {elf_raw_data_ + get_file_offset(), get_file_size()};
    }

  private:
    friend class ELF;

    Segment(unsigned char *elf_raw_data, ::Elf64_Phdr *program_header) noexcept
        : elf_raw_data_{elf_raw_data}, program_header_{program_header} {}

    unsigned char *elf_raw_data_;
    ::Elf64_Phdr *program_header_;
};

} // namespace elf

#endif // LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_SEGMENT_HPP
