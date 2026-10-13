#ifndef INCLUDE_UTILS_FD_WRAPPER_HPP
#define INCLUDE_UTILS_FD_WRAPPER_HPP

#include <cstddef>
#include <filesystem>
#include <format>
#include <ranges>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <unistd.h>

namespace utils {

namespace fs = std::filesystem;

class FDWrapper final {
  public:
    static constexpr int kInvalidFD = -1;

    enum class Flags : int {
        kReadOnly = O_RDONLY,
        kWriteOnly = O_WRONLY,
        kReadWrite = O_RDWR,
        kCreate = O_CREAT,
        kTruncate = O_TRUNC
    };

    explicit FDWrapper(const fs::path &path, Flags flags, fs::perms perms = fs::perms::none)
        : fd_{::open(path.c_str(), std::to_underlying(flags), static_cast<::mode_t>(perms))} {
        if (fd_ == kInvalidFD) {
            throw std::system_error{errno, std::system_category(),
                                    std::format("could not open '{}'", path.native())};
        }
    }

    FDWrapper(const FDWrapper &) = delete;
    FDWrapper &operator=(const FDWrapper &) = delete;

    FDWrapper(FDWrapper &&rhs) noexcept : fd_{std::exchange(rhs.fd_, kInvalidFD)} {}
    FDWrapper &operator=(FDWrapper &&rhs) noexcept {
        swap(rhs);
        return *this;
    }

    ~FDWrapper() { ::close(fd_); }

    int get() const noexcept { return fd_; }
    int release() noexcept { return std::exchange(fd_, kInvalidFD); }

    void swap(FDWrapper &other) noexcept { std::swap(fd_, other.fd_); }

  private:
    int fd_;
};

constexpr FDWrapper::Flags operator|(FDWrapper::Flags l, FDWrapper::Flags r) noexcept {
    return static_cast<FDWrapper::Flags>(std::to_underlying(l) | std::to_underlying(r));
}

constexpr FDWrapper::Flags &operator|=(FDWrapper::Flags &l, FDWrapper::Flags r) noexcept {
    return l = l | r;
}

constexpr FDWrapper::Flags operator&(FDWrapper::Flags l, FDWrapper::Flags r) noexcept {
    return static_cast<FDWrapper::Flags>(std::to_underlying(l) & std::to_underlying(r));
}

constexpr FDWrapper::Flags &operator&=(FDWrapper::Flags &l, FDWrapper::Flags r) noexcept {
    return l = l & r;
}

inline std::size_t write_all(FDWrapper &fd, unsigned char *data, std::size_t n,
                             std::size_t attempts = 4) {
    std::size_t overall_wrote = 0;

    for (const auto _ : std::views::iota(0uz, attempts)) {
        const ssize_t wrote = ::write(fd.get(), data + overall_wrote, n);
        if (wrote == -1 && errno != EINTR) {
            throw std::system_error{errno, std::system_category(), "write_all()"};
        } else if (const auto u_wrote = static_cast<std::size_t>(wrote); u_wrote != n) {
            overall_wrote += u_wrote;
            n -= u_wrote;
        } else {
            overall_wrote += u_wrote;
            break;
        }
    }

    return overall_wrote;
}

} // namespace utils

#endif // INCLUDE_UTILS_FD_WRAPPER_HPP
