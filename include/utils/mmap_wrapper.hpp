#ifndef INCLUDE_UTILS_MMAP_WRAPPER_HPP
#define INCLUDE_UTILS_MMAP_WRAPPER_HPP

#include <cstddef>
#include <filesystem>
#include <format>
#include <memory>
#include <system_error>

#include <sys/mman.h>
#include <sys/stat.h>

#include "utils/fd_wrapper.hpp"

namespace utils {

namespace fs = std::filesystem;

class ReadOnlyFileMapper final {
  public:
    using pointer = const unsigned char *;
    using iterator = pointer;
    using const_iterator = iterator;

    ReadOnlyFileMapper() = default;

    ReadOnlyFileMapper(int fd) : m_data{map_file(fd)} {}

    ReadOnlyFileMapper(const fs::path &path) : m_data{map_file(path)} {}

    pointer data() const noexcept { return m_data.get(); }

    std::size_t size() const noexcept { return m_data.get_deleter().size(); }
    bool empty() const noexcept { return !static_cast<bool>(m_data); }

    iterator begin() const noexcept { return data(); }
    iterator end() const noexcept { return data() + size(); }

    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }

  private:
    class Deleter final {
      public:
        Deleter() noexcept = default;
        explicit Deleter(std::size_t sz) noexcept : size_{sz} {}

        void operator()(unsigned char *ptr) const noexcept { ::munmap(ptr, size_); }

        std::size_t size() const noexcept { return size_; }

      private:
        std::size_t size_ = 0;
    };

    static std::unique_ptr<unsigned char, Deleter> map_file(const fs::path &path) {
        /*
         * man mmap:
         * After the mmap() call has returned, the file descriptor, fd, can be closed immediately
         * without invalidating the mapping
         */
        const FDWrapper fd{path, FDWrapper::Flags::kReadOnly};
        return map_file(fd.get());
    }

    static std::unique_ptr<unsigned char, Deleter> map_file(int fd) {
        struct stat statbuf;
        if (::fstat(fd, &statbuf) == -1) {
            throw std::system_error{errno, std::system_category(),
                                    "utils::ReadOnlyFileMapper::map_file(): fstat() failed"};
        }

        if (S_ISDIR(statbuf.st_mode)) {
            throw std::invalid_argument{
                std::format("trying to map a directory \'{}\'",
                            fs::read_symlink(std::format("/proc/self/{}", fd)).native())};
        } else if (!S_ISREG(statbuf.st_mode)) {
            throw std::invalid_argument{
                std::format("trying to map an irregular file \'{}\'",
                            fs::read_symlink(std::format("/proc/self/{}", fd)).native())};
        }

        if (const auto size = static_cast<std::size_t>(statbuf.st_size)) {
            constexpr ::off_t kFileBeginOffset = 0;
            // Use NULL instead of nullptr, because they are not necessarily represented identically
            auto *data = static_cast<unsigned char *>(
                ::mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, kFileBeginOffset));
            if (data == MAP_FAILED) {
                throw std::system_error{errno, std::system_category(),
                                        std::format("mmap(/*addr=*/NULL, /*size=*/{}, PROT_READ, "
                                                    "MAP_PRIVATE, /*fd=*/{}, /*offset=*/0)",
                                                    size, fd)};
            }

            return {data, Deleter{size}};
        }

        return {};
    }

    std::unique_ptr<unsigned char, Deleter> m_data;
};

} // namespace utils

#endif // INCLUDE_UTILS_MMAP_WRAPPER_HPP
