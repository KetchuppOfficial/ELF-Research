#include <cstdlib>
#include <exception>
#include <expected>
#include <map>
#include <print>
#include <ranges>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>
#include <benchmark/benchmark.h>
#include <elfio/elfio.hpp>

#include "common/libraries.hpp"

#include "generator/config.hpp"
#include "generator/elfio.hpp"
#include "generator/libelf.hpp"

namespace {

struct Options {
    elf::Libs lib{};
    std::filesystem::path file_path;
    std::filesystem::path config_path;
    bool use_mmap = false;
    std::vector<std::string> benchmark_args;
};

std::expected<Options, int> parse_options(int argc, char **argv) {
    Options opts;

    CLI::App app{"A simple application for generation of ELF files"};
    app.allow_extras();

    using enum elf::Libs;
    const std::map<std::string, elf::Libs> kLibs{{"elfio", kELFIO}, {"libelf", kLibELF}};
    app.add_option("--lib", opts.lib)->transform(CLI::CheckedTransformer(kLibs))->required();

    app.add_flag(
        "--use-mmap", opts.use_mmap,
        "pass hints to the libelf internals to use mmap; can only be used with --lib libelf");

    app.add_option("-o,--output", opts.file_path, "output file name")->required();

    app.add_option("--config", opts.config_path, "path to the config")->required();

    try {
        app.parse(argc, argv);
        if (opts.lib != kLibELF && opts.use_mmap) {
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
    auto options_or_error = parse_options(argc, argv);
    if (!options_or_error.has_value()) {
        return options_or_error.error();
    }
    auto &opts = *options_or_error;

    std::ifstream file{opts.config_path};

    std::string json(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
    if (!file) {
        std::println(stderr, "reading json failed");
        return EXIT_FAILURE;
    }

    const elf::Config config{json};

    const auto generator = [&opts, &config] -> std::unique_ptr<elf::Generator> {
        using enum elf::Libs;
        switch (opts.lib) {
        case kELFIO:
            return std::make_unique<elf::ELFIOGenerator>(config);
        case kLibELF:
            return std::make_unique<elf::LibELFGenerator>(config, opts.use_mmap);
        default:
            std::unreachable();
        }
    }();

    benchmark::RegisterBenchmark("generator", [&](benchmark::State &state) {
        for (auto _ : state) {
            { // because creating a file from scratch is faster
                state.PauseTiming();
                std::filesystem::remove(opts.file_path);
                state.ResumeTiming();
            }
            generator->generate(opts.file_path);
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
