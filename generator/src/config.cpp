#include <algorithm>
#include <cstddef>
#include <format>
#include <set>
#include <stdexcept>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "generator/config.hpp"

namespace elf {

namespace {

struct Compare {
    static bool operator()(const Config::Region &lhs, const Config::Region &rhs) noexcept {
        return lhs.addr + lhs.size <= rhs.addr;
    }
};

bool exists_intersection(const std::vector<Config::Region> &regions) {
    std::set<Config::Region, Compare> set;
    for (const auto &reg : regions) {
        if (!set.insert(reg).second) {
            return true;
        }
    }
    return false;
}

} // namespace

Config::Config(std::string_view json_str) {
    nlohmann::json_schema::json_validator validator;
    validator.set_root_schema(nlohmann::json::parse(kSchema));

    auto json = nlohmann::json::parse(json_str);
    validator.validate(json);

    auto fill = [&json](std::string_view name, std::vector<Region> &regions) {
        for (const auto &sec : json[name]) {
            regions.emplace_back(sec["addr"], sec["size"]);
        }
        if (exists_intersection(regions)) {
            throw std::invalid_argument{std::format("{} intersect", name)};
        }
    };

    entry_point_ = json["entry_point"].get<std::size_t>();
    fill("sections", sections_);
    fill("segments", segments_);

    if (std::ranges::none_of(segments_, [entry = entry_point_](const Region &region) {
            return region.addr <= entry && entry < region.addr + region.size;
        })) {
        throw std::invalid_argument{std::format("entry point does not belong to any segment")};
    }
}

} // namespace elf
