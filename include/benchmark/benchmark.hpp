#ifndef INCLUDE_BENCHMARK_BENCHMARK_HPP
#define INCLUDE_BENCHMARK_BENCHMARK_HPP

#include <chrono>
#include <filesystem>
#include <string_view>

namespace elf {

namespace fs = std::filesystem;

class Benchmark {
  public:
    using ms = std::chrono::milliseconds;

    struct Result {
        ms load;
        ms save;
    };

    virtual ~Benchmark() = default;

    virtual Result run(const fs::path &path) const = 0;

    static fs::path path_to_copy(fs::path path) {
        using namespace std::string_view_literals;
        constexpr auto kPrefix = "-copy"sv;
        path = path.lexically_normal();
        if ((fs::status(path.parent_path()).permissions() & fs::perms::others_write) !=
            fs::perms::none) {
            return std::move(path += kPrefix);
        }
        return std::move(fs::current_path() /= path.filename() += kPrefix);
    }
};

} // namespace elf

#endif // INCLUDE_BENCHMARK_BENCHMARK_HPP
