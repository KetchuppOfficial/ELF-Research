#ifndef INCLUDE_LIBELF_PLUS_PLUS_DATA_HPP
#define INCLUDE_LIBELF_PLUS_PLUS_DATA_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>

namespace elf {

class Data final {
  public:
    explicit Data(std::size_t size) : size_{size}, data_{new unsigned char[size]} {}

    template <std::forward_iterator It>
        requires std::same_as<unsigned char, std::iter_value_t<It>>
    Data(It first, It last) : Data{std::distance(first, last)} {
        std::copy(first, last, data_.get());
    }

    template <std::ranges::forward_range R>
        requires std::same_as<unsigned char, std::ranges::range_value_t<R>>
    Data(R &&range) : Data{std::ranges::size(range)} {
        std::ranges::copy(std::forward<R>(range), data_.get());
    }

    std::size_t size() const noexcept { return size_; }

    unsigned char *data() const noexcept { return data_.get(); }

  private:
    std::size_t size_;
    std::unique_ptr<unsigned char[]> data_;
};

} // namespace elf

#endif // INCLUDE_LIBELF_PLUS_PLUS_DATA_HPP
