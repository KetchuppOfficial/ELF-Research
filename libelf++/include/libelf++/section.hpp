#ifndef INCLUDE_LIBELF_PLUS_PLUS_SECTION_HPP
#define INCLUDE_LIBELF_PLUS_PLUS_SECTION_HPP

#include <span>
#include <utility>

#include <libelf.h>

namespace elf {

class Section final {
  public:
    enum Type : ::Elf64_Word { kLoad = PT_LOAD };

    ::Elf64_Word get_name_index() const noexcept { return section_header_->sh_name; }

    Type get_type() const noexcept { return Type{section_header_->sh_type}; }
    void set_type(Type type) noexcept { section_header_->sh_type = std::to_underlying(type); }

    ::Elf64_Xword get_flags() const noexcept { return section_header_->sh_flags; }
    void set_flags(::Elf64_Xword flags) noexcept { section_header_->sh_flags = flags; }

    ::Elf64_Addr get_virt_addr() const noexcept { return section_header_->sh_addr; }

    ::Elf64_Off get_file_offset() const noexcept { return section_header_->sh_offset; }

    ::Elf64_Xword get_size() const noexcept { return section_header_->sh_size; }

    ::Elf64_Word get_link() const noexcept { return section_header_->sh_link; }

    ::Elf64_Word get_info() const noexcept { return section_header_->sh_info; }

    ::Elf64_Xword get_alignment() const noexcept { return section_header_->sh_addralign; }

    ::Elf64_Xword get_entry_size() const noexcept { return section_header_->sh_entsize; }

    std::span<unsigned char> data() { return {elf_raw_data_ + get_file_offset(), get_size()}; }
    std::span<const unsigned char> data() const {
        return {elf_raw_data_ + get_file_offset(), get_size()};
    }

  private:
    friend class ELF;

    Section(unsigned char *elf_raw_data, ::Elf64_Shdr *section_header) noexcept
        : elf_raw_data_{elf_raw_data}, section_header_{section_header} {}

    unsigned char *elf_raw_data_;
    ::Elf64_Shdr *section_header_;
};

} // namespace elf

#endif // INCLUDE_LIBELF_PLUS_PLUS_SECTION_HPP
