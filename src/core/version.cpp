#include "nizaw/core/version.hpp"

#include <string>

namespace nizaw::core {

std::string version_string() {
    return std::to_string(kVersionMajor) + "." + std::to_string(kVersionMinor) + "." +
           std::to_string(kVersionPatch);
}

Version version() noexcept {
    return {kVersionMajor, kVersionMinor, kVersionPatch};
}

}  // namespace nizaw::core
