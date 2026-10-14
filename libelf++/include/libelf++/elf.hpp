#ifndef LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_ELF_HPP
#define LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_ELF_HPP

#include <algorithm>
#include <filesystem>
#include <format>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gelf.h>
#include <libelf.h>

#include "libelf++/data.hpp"
#include "libelf++/error.hpp"
#include "libelf++/header.hpp"
#include "libelf++/section.hpp"
#include "libelf++/segment.hpp"

#include "utils/fd_wrapper.hpp"

namespace elf {

namespace fs = std::filesystem;

class ELF final {
  public:
    struct UseMMap {
        explicit UseMMap() = default;
    };

    explicit ELF([[maybe_unused]] UseMMap, const fs::path &path, utils::FDWrapper::Flags flags,
                 fs::perms perms = fs::perms::none)
        : ELF{path, flags, file_flag_to_elf_flag_with_mmap(flags), perms} {}

    explicit ELF(const fs::path &path, utils::FDWrapper::Flags flags,
                 fs::perms perms = fs::perms::none)
        : ELF{path, flags, file_flag_to_elf_flag(flags), perms} {}

    ~ELF() { ::elf_end(elf_); }

    ELF(const ELF &) = delete;
    ELF &operator=(const ELF &) = delete;

    ELF(ELF &&rhs) : elf_{std::exchange(rhs.elf_, nullptr)}, fd_{std::move(rhs.fd_)} {}
    ELF &operator=(ELF &&rhs) {
        swap(rhs);
        return *this;
    }

    Elf *get() noexcept { return elf_; }

    void save() {
        if (const auto written = ::elf_update(elf_, ELF_C_WRITE_MMAP); written == -1) {
            throw std::system_error{::elf_errno(), libelf_category(), "elf_update"};
        }
    }

    Header get_header() const { return Header{*do_get_header()}; }

    // Obtains the real number of segments if it is to big to fit into e_phnum field of the header
    std::size_t real_segments_count() const {
        std::size_t segments_count;
        if (::elf_getphdrnum(elf_, &segments_count) != 0) {
            throw std::system_error{::elf_errno(), libelf_category(), "elf_getphdrnum"};
        }
        return segments_count;
    }

    // Obtains the real number of sections if it is to big to fit into e_shnum field of the header
    std::size_t real_sections_count() const {
        std::size_t sections_count;
        if (::elf_getshdrnum(elf_, &sections_count) != 0) {
            throw std::system_error{::elf_errno(), libelf_category(), "elf_getshdrnum"};
        }
        return sections_count;
    }

    Segment get_segment(std::size_t index) const {
        if (const auto segments_count = real_segments_count(); index >= segments_count) {
            throw std::out_of_range{
                std::format("attempt to access segment with index {}/{}", index, segments_count)};
        }

        // NOTE: This ordering of calls to get_raw_data() and elf64_getphdr() may reduce the number
        // of calls to malloc()
        auto *elf_data = get_raw_data();
        auto *program_header_table = ::elf64_getphdr(elf_);
        return {elf_data, &program_header_table[index]};
    }

    std::ranges::view auto segments() const {
        // See note above
        auto *elf_data = get_raw_data();
        auto *program_header_table = ::elf64_getphdr(elf_);
        return std::views::iota(0uz, real_segments_count()) |
               std::views::transform(
                   [=](std::size_t i) { return Segment{elf_data, &program_header_table[i]}; });
    }

    std::ranges::view auto sections() const {
        auto *elf_data = reinterpret_cast<unsigned char *>(::elf_rawfile(elf_, nullptr));
        return std::views::iota(0uz, real_sections_count()) |
               std::views::transform([elf = elf_, elf_data](std::size_t i) {
                   auto *section_header = ::elf64_getshdr(::elf_getscn(elf, i));
                   return Section{elf_data, section_header};
               });
    }

    template <typename Self>
    auto raw_contents(this Self &&self) -> std::span<
        std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const char, char>> {
        size_t n_bytes{};
        char *ptr = ::elf_rawfile(self.elf_, &n_bytes);
        return {ptr, n_bytes};
    }

    void swap(ELF &other) noexcept {
        std::swap(elf_, other.elf_);
        fd_.swap(other.fd_);
    }

    void add_data(::Elf_Scn *section, const Data &data, ::Elf_Type type,
                  std::size_t alignment = 0x10) {
        ::Elf_Data *elf_data = ::elf_newdata(section);
        if (!elf_data) {
            throw std::system_error{::elf_errno(), libelf_category(), "elf_newdata"};
        }
        elf_data->d_buf = data.data();
        elf_data->d_type = type;
        elf_data->d_size = data.size();
        elf_data->d_align = alignment;
    }

    void add_data(::Elf_Scn *section, Data &&data, ::Elf_Type type, std::size_t alignment = 0x10) {
        add_data(section, static_cast<const Data &>(data), type, alignment);
        m_data_regions.emplace_back(std::move(data));
    }

  private:
    explicit ELF(const fs::path &path, utils::FDWrapper::Flags flags, ::Elf_Cmd elf_flags,
                 fs::perms perms = fs::perms::none)
        : fd_{path, flags, perms} {
        init();
        elf_ = ::elf_begin(fd_.get(), elf_flags, NULL);
        if (!elf_) {
            throw std::system_error{::elf_errno(), libelf_category(), "elf_begin"};
        }
        if (std::to_underlying(flags & utils::FDWrapper::Flags::kWriteOnly)) {
            auto *header = do_create_header();
            header->e_ehsize = header->e_phoff = 0x40;
            header->e_version = EV_CURRENT;
        }
    }

    static ::Elf_Cmd file_flag_to_elf_flag_with_mmap(utils::FDWrapper::Flags flags) {
        using enum utils::FDWrapper::Flags;
        if (flags == kReadOnly) {
            return ELF_C_READ_MMAP;
        } else if ((flags & kWriteOnly) == kWriteOnly) {
            return ELF_C_WRITE_MMAP;
        } else if ((flags & kReadWrite) == kReadWrite) {
            return ELF_C_RDWR_MMAP;
        } else [[unlikely]] {
            throw std::invalid_argument(
                std::format("invalid flag {} passed to ELF", std::to_underlying(flags)));
        }
    }

    static ::Elf_Cmd file_flag_to_elf_flag(utils::FDWrapper::Flags flags) {
        using enum utils::FDWrapper::Flags;
        if (flags == kReadOnly) {
            return ELF_C_READ;
        } else if ((flags & kWriteOnly) == kWriteOnly) {
            return ELF_C_WRITE;
        } else if ((flags & kReadWrite) == kReadWrite) {
            return ELF_C_RDWR;
        } else [[unlikely]] {
            throw std::invalid_argument(
                std::format("invalid flag {} passed to ELF", std::to_underlying(flags)));
        }
    }

    void init() const {
        if (::elf_version(EV_CURRENT) == EV_NONE) {
            throw std::system_error{::elf_errno(), libelf_category(), "elf_version"};
        }
    }

    ::Elf64_Ehdr *do_get_header() const {
        if (::Elf64_Ehdr *header = ::elf64_getehdr(elf_)) {
            return header;
        }
        throw std::system_error{::elf_errno(), libelf_category(), "elf64_getehdr"};
    }

    ::Elf64_Ehdr *do_create_header() {
        if (::Elf64_Ehdr *header = ::elf64_newehdr(elf_)) {
            return header;
        }
        throw std::system_error{::elf_errno(), libelf_category(), "elf64_newehdr"};
    }

    unsigned char *get_raw_data() const noexcept {
        return reinterpret_cast<unsigned char *>(::elf_rawfile(elf_, nullptr));
    }

    ::Elf *elf_;
    utils::FDWrapper fd_;
    std::vector<Data> m_data_regions;
};

} // namespace elf

#endif // LIBELF_PLUS_PLUS_INCLUDE_LIBELF_PLUS_PLUS_ELF_HPP
