#ifndef INCLUDE_LIBELF_PLUS_PLUS_ELF_HPP
#define INCLUDE_LIBELF_PLUS_PLUS_ELF_HPP

#include <algorithm>
#include <filesystem>
#include <format>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gelf.h>
#include <libelf.h>

#include "libelf++/data.hpp"
#include "libelf++/header.hpp"

#include "utils/fd_wrapper.hpp"

namespace elf {

namespace fs = std::filesystem;

using namespace std::string_view_literals;

class ELF final {
  public:
    explicit ELF(const fs::path &path, utils::FDWrapper::Flags flags,
                 fs::perms perms = fs::perms::none)
        : fd_{path, flags, perms} {
        init();
        elf_ = ::elf_begin(fd_.get(), file_flag_to_elf_flag(flags), NULL);
        if (!elf_) {
            throw std::runtime_error{elf_error("elf_begin"sv)};
        }
        auto *header = do_get_header();
        header->e_ehsize = header->e_phoff = 0x40;
    }

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
            throw std::runtime_error{elf_error("elf_update"sv)};
        }
    }

    Header get_header() { return Header{*do_get_header()}; }

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
            throw std::runtime_error{elf_error("elf_newdata"sv)};
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
    void init() const {
        if (::elf_version(EV_CURRENT) == EV_NONE) {
            throw std::runtime_error{elf_error("elf_version"sv)};
        }
    }

    ::Elf64_Ehdr *do_get_header() {
        if (::Elf64_Ehdr *header = ::elf64_newehdr(elf_)) {
            return header;
        }
        throw std::runtime_error{elf_error("elf64_newehdr"sv)};
    }

    static std::string elf_error(std::string_view func) {
        return std::format("{}: {}", func, ::elf_errmsg(::elf_errno()));
    }

    static ::Elf_Cmd file_flag_to_elf_flag(utils::FDWrapper::Flags flags) {
        using enum utils::FDWrapper::Flags;
        ::Elf_Cmd result;
        if (flags == kReadOnly) {
            result = ELF_C_READ_MMAP;
        } else if ((flags & kWriteOnly) == kWriteOnly) {
            result = ELF_C_WRITE_MMAP;
        } else if ((flags & kReadWrite) == kReadWrite) {
            result = ELF_C_RDWR_MMAP;
        } else [[unlikely]] {
            throw std::invalid_argument(
                std::format("invalid flag {} passed to ELF", std::to_underlying(flags)));
        }

        return result;
    }

    ::Elf *elf_;
    utils::FDWrapper fd_;
    std::vector<Data> m_data_regions;
};

} // namespace elf

#endif // INCLUDE_LIBELF_PLUS_PLUS_ELF_HPP
