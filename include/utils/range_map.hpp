#ifndef INCLUDE_UTILS_RANGE_MAP_HPP
#define INCLUDE_UTILS_RANGE_MAP_HPP

#include <cstdint>
#include <map>

namespace utils {

struct InclusiveRange {
    std::uint64_t start;
    std::uint64_t size;
};

struct Compare {
    bool operator()(const InclusiveRange &lhs, const InclusiveRange &rhs) const noexcept {
        return lhs.start + lhs.size <= rhs.start;
    }
};

template<typename T>
using RangeMap = std::map<InclusiveRange, T, Compare>;

} // namespace utils

#endif // INCLUDE_UTILS_RANGE_MAP_HPP
