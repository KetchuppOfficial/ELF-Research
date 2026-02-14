#include <cstdio>
#include <cstdlib>
#include <exception>
#include <map>
#include <print>
#include <string>

#include <CLI/CLI.hpp>

#include <elfio/elfio.hpp>

#include "common/libraries.hpp"

#include "generator/elfio.hpp"
#include "generator/libelf.hpp"

int main(int argc, char **argv) try {
    elf::Generator::Config config{};

    CLI::App app{"A simple application for generation of ELF files with random contents"};

    using enum elf::Libs;
    const std::map<std::string, elf::Libs> kLibs{
        {"elfio", kELFIO}, {"lief", kLIEF}, {"libelf", kLibELF}};
    elf::Libs lib;
    app.add_option("--lib", lib)->transform(CLI::CheckedTransformer(kLibs))->required();

    app.add_option("-o,--output", config.file_name, "output file name")->required();

    app.add_option("--n-segments", config.segments_count,
                   "the number of segments in the generated binary")
        ->required();

    app.add_option("--segment-size", config.segment_size, "average size of a segment in bytes")
        ->default_val(elf::Generator::Config::kPageSize);

    app.add_option("--entry", config.entry_point, "entry point of the executable")
        ->default_val(elf::Generator::Config::kDefaultEntryPoint);

    CLI11_PARSE(app, argc, argv);

    const auto generator = [&] -> std::unique_ptr<elf::Generator> {
        switch (lib) {
        case kELFIO:
            return std::make_unique<elf::ELFIOGenerator>();
        case kLibELF:
            return std::make_unique<elf::LibELFGenerator>();
        case kLIEF:
            throw std::runtime_error{"lief support is not implemented"};
        default:
            std::unreachable();
        }
    }();

    generator->generate(config);
} catch (const std::exception &e) {
    std::println(stderr, "Caught an instance of {}\nwhat(): {}", typeid(e).name(), e.what());
    return EXIT_FAILURE;
} catch (...) {
    std::println(stderr, "Caught an unknown exception");
    return EXIT_FAILURE;
}
