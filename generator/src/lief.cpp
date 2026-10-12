#include <print>
#include <ranges>

#include <LIEF/LIEF.hpp>

#include "generator/lief.hpp"

#include <bfd.h>

namespace elf {

void LIEFGenerator::generate(const std::filesystem::path &file_path, const Config &config) const {
    static const std::vector<std::uint8_t> kEmptyELF = {
#embed "single-segment.x"
    };

    const std::vector<unsigned char> data(config.segment_size, 'Z');

#if 0
    LIEF::logging::enable();
    LIEF::logging::set_level(LIEF::logging::LEVEL::TRACE);
#endif

    auto elf = LIEF::ELF::Parser::parse(kEmptyELF);

    {
        using LIEF::ELF::Header;
        auto &header = elf->header();
        header.identity_class(Header::CLASS::ELF64);
        header.identity_data(Header::ELF_DATA::LSB);
        header.file_type(Header::FILE_TYPE::EXEC);
        header.machine_type(LIEF::ELF::ARCH::X86_64);
        header.entrypoint(config.entry_point);
    }

    std::size_t addr = config.entry_point;

    using LIEF::ELF::Segment;

    for (const auto _ : std::views::iota(0uz, config.segments_count)) {
#if 0
        auto *sec = [&] {
            using LIEF::ELF::Binary;
            using LIEF::ELF::Section;

            Section sec;
            sec.type(Section::TYPE::PROGBITS);
            sec.flags(std::to_underlying(Section::FLAGS::ALLOC | Section::FLAGS::EXECINSTR));
            sec.alignment(0x10);
            sec.content(data);
            std::println("{}", data.size());
            sec.virtual_address(addr);

            return elf->add(sec, /*loaded=*/true);
        }();

        if (!sec) {
            throw std::runtime_error{"could not create a section"};
        }
#endif
#if 1
        {
            Segment seg;
            seg.type(Segment::TYPE::LOAD);
            seg.flags(Segment::FLAGS::R | Segment::FLAGS::X);
            seg.physical_address(addr);
            seg.virtual_address(addr);
            seg.physical_size(config.segment_size);
            seg.virtual_size(config.segment_size);
            seg.alignment(config.segment_alignment);
            seg.content(data);
            if (!elf->add(seg, addr)) {
                throw std::runtime_error{"could not create a segment"};
            }
        }
#endif
        addr = align_up(addr + config.segment_size, config.segment_alignment);
    }

    std::vector<Segment *> segments;
    for (auto &seg : elf->segments()) {
        if (seg.flags() != (Segment::FLAGS::R | Segment::FLAGS::X)) {
            segments.push_back(&seg);
        }
    }

    auto *sec = elf->get_section("dummy");

    for (auto &seg : sec->segments()) {
        segments.push_back(&seg);
    }

    for (auto *seg : segments) {
        elf->remove(*seg);
    }

    elf->remove(*sec);

    {
        LIEF::ELF::Builder builder(*elf);
        builder.build();
    }

    elf->write(config.file_name.native());
}

} // namespace elf
