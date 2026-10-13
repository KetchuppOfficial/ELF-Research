#include <cstdlib>
#include <exception>
#include <expected>
#include <filesystem>
#include <memory>
#include <print>
#include <string>
#include <utility>

#include <CLI/CLI.hpp>
#include <benchmark/benchmark.h>

#include "loader/elfio.hpp"
#include "loader/libelf.hpp"
#include "loader/lief.hpp"
#include "loader/llvm.hpp"

#include "common/libraries.hpp"
#include "utils/enum_support.hpp"

namespace {

template <typename E>
    requires std::is_enum_v<E>
static constexpr auto kStringEnumPairs = [] consteval {
    std::array<std::pair<std::string_view, E>, enumerators_of(^^E).size()> res{};
    std::size_t i = 0;
    template for (constexpr auto pair : std::define_static_array(
                      enumerators_of(^^E) | std::views::transform([](std::meta::info e) {
                          return std::pair{e, std::meta::reflect_constant_string(
                                                  utils::identifier_of_google_enumerator(e))};
                      }))) {
        res[i++] = {[:pair.second:], [:pair.first:]};
    }
    return res;
}();

struct Options {
    std::filesystem::path file_name;
    elf::Libs lib{};
    bool use_mmap;
    std::vector<std::string> benchmark_args;
};

std::expected<Options, int> parse_options(int argc, char **argv) {
    CLI::App app{"Benchmark for comparing ELF parsing time for different libraries"};
    app.allow_extras();

    Options opts;

    app.add_option("input", opts.file_name);

    app.add_option("--lib", opts.lib)
        ->transform(CLI::CheckedTransformer(kStringEnumPairs<elf::Libs>))
        ->required();

    app.add_flag(
        "--use-mmap", opts.use_mmap,
        "pass hints to the libelf internals to use mmap; can only be used with --lib libelf");

    try {
        app.parse(argc, argv);
        if (opts.lib != elf::Libs::kLibELF && opts.use_mmap) {
            throw CLI::ValidationError("--use-mmap", "can only be used with --lib libelf");
        }
    } catch (const CLI::ParseError &e) {
        return std::unexpected{app.exit(e)};
    }

    opts.benchmark_args = app.remaining();
    opts.benchmark_args.emplace(opts.benchmark_args.begin(), argv[0]);

    return {std::move(opts)};
}

} // unnamed namespace

int main(int argc, char **argv) try {
    std::expected options_or_error = parse_options(argc, argv);
    if (!options_or_error.has_value()) {
        return options_or_error.error();
    }
    auto &opts = *options_or_error;

    const auto loader = [&] -> std::unique_ptr<elf::Loader> {
        using enum elf::Libs;
        switch (opts.lib) {
        case kELFIO:
            return std::make_unique<elf::ELFIOLoader>();
        case kLIEF:
            return std::make_unique<elf::LIEFLoader>();
        case kLibELF:
            return std::make_unique<elf::LibELFLoader>(opts.use_mmap);
        case kLLVM:
            return std::make_unique<elf::LLVMLoader>();
        default:
            std::unreachable();
        }
    }();

    benchmark::RegisterBenchmark("loader", [&](benchmark::State &state) {
        for (auto _ : state) {
            loader->run(opts.file_name);
            benchmark::ClobberMemory();
        }
    });

    {
        int benchmark_argc = opts.benchmark_args.size();
        std::vector<char *> benchmark_argv{
            std::from_range,
            opts.benchmark_args | std::views::transform([](auto &str) { return str.data(); })};
        benchmark::Initialize(&benchmark_argc, benchmark_argv.data());
        if (benchmark::ReportUnrecognizedArguments(benchmark_argc, benchmark_argv.data())) {
            return EXIT_FAILURE;
        }
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
} catch (const std::exception &e) {
    std::println(stderr, "Caught an instance of {}\nwhat(): {}", typeid(e).name(), e.what());
    return EXIT_FAILURE;
} catch (...) {
    std::println(stderr, "Caught an unknown exception");
    return EXIT_FAILURE;
}
