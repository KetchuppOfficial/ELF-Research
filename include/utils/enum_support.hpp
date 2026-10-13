#ifndef INCLUDE_UTILS_ENUM_SUPPORT_HPP
#define INCLUDE_UTILS_ENUM_SUPPORT_HPP

#include <meta>
#include <ranges>
#include <string_view>
#include <type_traits>

namespace utils {

template <typename E,
          auto F = [](std::meta::info e) consteval { return std::meta::identifier_of(e); }>
    requires std::is_enum_v<E>
constexpr std::string_view enum_to_str(E value) {
    template for (constexpr auto e : std::define_static_array(enumerators_of(^^E))) {
        if (value == [:e:]) {
            return F(e);
        }
    }

    return "<unnamed>";
}

consteval auto identifier_of_google_enumerator(std::meta::info e) {
    std::string_view initial = identifier_of(e);
    if (initial.front() != 'k') {
        throw std::meta::exception("enumerator does not start with 'k'", e);
    }

    const char *str = std::define_static_string(initial | std::views::drop(1) |
                                                std::views::transform([](char c) consteval -> char {
                                                    if ('A' <= c && c <= 'Z') {
                                                        return 'a' + (c - 'A');
                                                    }
                                                    return c;
                                                }));
    return std::string_view{str, initial.size() - 1};
}

static_assert([] consteval {
    enum Color { kRed };
    return identifier_of_google_enumerator(^^Color::kRed) == "red";
}());

static_assert([] consteval -> bool {
    enum Color { red };
    try {
        static_cast<void>(identifier_of_google_enumerator(^^Color::red));
        return false;
    } catch (const std::meta::exception &e) {
        return true;
    }
});

} // namespace utils

#endif // INCLUDE_UTILS_ENUM_SUPPORT_HPP
