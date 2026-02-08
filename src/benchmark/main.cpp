#include <cstdlib>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <ostream>
#include <print>
#include <string>
#include <utility>

#include <CLI/CLI.hpp>

#include "benchmark/elfio.hpp"
#include "benchmark/lief.hpp"

enum class Libs { kELFIO, kLIEF };

namespace {

std::string_view to_string_view(Libs lib) {
    using enum Libs;
    using namespace std::string_view_literals;
    switch (lib) {
    case kELFIO:
        return "elfio"sv;
    case kLIEF:
        return "lief"sv;
    default:
        std::unreachable();
    }
}

} // unnamed namespace

// Needed for pretty help message by CLI11
std::ostream &operator<<(std::ostream &os, Libs lib) { return os << to_string_view(lib); }

int main(int argc, char **argv) try {
    CLI::App app{"Benchmark comparing different ELF-parsing libraries"};

    std::filesystem::path file_name;
    app.add_option("input", file_name);

    const std::map<std::string, Libs> kLibs{{"elfio", Libs::kELFIO}, {"lief", Libs::kLIEF}};
    Libs lib;
    app.add_option("--lib", lib)->transform(CLI::CheckedTransformer(kLibs))->required();

    CLI11_PARSE(app, argc, argv);

    const auto bench = [lib] -> std::unique_ptr<elf::Benchmark> {
        switch (lib) {
        case Libs::kELFIO:
            return std::make_unique<elf::ELFIOBenchmark>();
        case Libs::kLIEF:
            return std::make_unique<elf::LIEFBenchmark>();
        default:
            std::unreachable();
        }
    }();

    const auto [load_time, save_time] = bench->run(file_name);

    std::println("load: {} ms", load_time.count());
    std::println("save: {} ms", save_time.count());

} catch (const std::exception &e) {
    std::println(stderr, "Caught an instance of {}\nwhat(): {}", typeid(e).name(), e.what());
    return EXIT_FAILURE;
} catch (...) {
    std::println(stderr, "Caught an unknown exception");
    return EXIT_FAILURE;
}
