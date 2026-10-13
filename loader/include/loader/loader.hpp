#ifndef LOADER_INCLUDE_LOADER_LOADER_HPP
#define LOADER_INCLUDE_LOADER_LOADER_HPP

#include <filesystem>

namespace elf {

namespace fs = std::filesystem;

class Loader {
  public:
    virtual ~Loader() = default;

    virtual void run(const fs::path &path) const = 0;
};

} // namespace elf

#endif // LOADER_INCLUDE_LOADER_LOADER_HPP
