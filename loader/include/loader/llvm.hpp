#ifndef INCLUDE_LOADER_LLVM_HPP
#define INCLUDE_LOADER_LLVM_HPP

#include "loader/loader.hpp"

namespace elf {

class LLVMLoader final : public Loader {
public:
    virtual void run(const fs::path &path) const override;
};

} // namespace

#endif // INCLUDE_BENCHMARK_LLVM_HPP
