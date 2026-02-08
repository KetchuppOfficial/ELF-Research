#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <print>
#include <random>
#include <ranges>
#include <string>
#include <type_traits>

#include <CLI/CLI.hpp>

#include <elfio/elfio.hpp>

namespace {

struct Config {
    static constexpr std::size_t kPageSize = 0x1000;
    static constexpr std::make_signed_t<std::size_t> kPageSizeDelta = 0x100;
    static constexpr std::size_t kInitialAddr = 4 * kPageSize;

    std::string file_name;
    std::size_t segments_count;
    std::size_t segment_avg_size;
};

constexpr std::size_t align_up(std::size_t value, std::size_t alignment) {
    return ((value / alignment) + (value % alignment != 0)) * alignment;
}

static_assert(align_up(0x1000 + 0x16, 0x1000) == 0x2000);
static_assert(align_up(0x1000, 0x1000) == 0x1000);

void generate_elf(const Config &config) {
    ELFIO::elfio elf;
    elf.create(ELFIO::ELFCLASS64, ELFIO::ELFDATA2LSB);
    elf.set_type(ELFIO::ET_EXEC);
    elf.set_machine(ELFIO::EM_X86_64);
    elf.set_entry(Config::kInitialAddr);

    std::mt19937_64 gen{std::random_device{}()};
    std::uniform_int_distribution<long> size_delta{-Config::kPageSizeDelta, Config::kPageSizeDelta};

    std::vector<char> data(config.segment_avg_size + Config::kPageSizeDelta);
    std::ifstream dev_random("/dev/random");

    for (std::size_t addr = Config::kInitialAddr;
         const auto i : std::views::iota(0uz, config.segments_count)) {
        const auto size = static_cast<std::size_t>(config.segment_avg_size + size_delta(gen));

        dev_random.read(data.data(), size);

        const auto *section = [&] {
            auto *section = elf.sections.add(std::format(".text.{}", i));
            section->set_type(ELFIO::SHT_PROGBITS);
            section->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_EXECINSTR);
            section->set_addr_align(0x10);
            section->set_data(data.data(), size);
            return section;
        }();

        auto *segment = [&] {
            auto *segment = elf.segments.add();
            segment->set_type(ELFIO::PT_LOAD);
            segment->set_flags(ELFIO::PF_R | ELFIO::PF_X);
            segment->set_physical_address(addr);
            segment->set_virtual_address(addr);
            segment->set_file_size(size);
            segment->set_memory_size(size);
            segment->set_align(Config::kPageSize);
            return segment;
        }();

        segment->add_section_index(section->get_index(), section->get_addr_align());
        addr = align_up(addr + size, Config::kPageSize);
    }

    elf.save(config.file_name);
}

} // unnamed namespace

int main(int argc, char **argv) try {
    Config config{};

    CLI::App app{"A simple application for generation of ELF files with random contents"};

    app.add_option("-o,--output", config.file_name, "output file name")->required();

    app.add_option("--n-segments", config.segments_count,
                   "the number of segments in the generated binary")
        ->required();

    app.add_option("--segment-size", config.segment_avg_size, "average size of a segment in bytes")
        ->default_val(Config::kPageSize);

    CLI11_PARSE(app, argc, argv);

    generate_elf(config);
} catch (const std::exception &e) {
    std::println(stderr, "Caught an instance of {}\nwhat(): {}", typeid(e).name(), e.what());
    return EXIT_FAILURE;
} catch (...) {
    std::println(stderr, "Caught an unknown exception");
    return EXIT_FAILURE;
}
