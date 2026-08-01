#pragma once

#include <string>
#include <sys/types.h>
#include <vector>

#include "nizaw/result.hpp"

namespace nizaw::security {

struct Identity {
    uid_t real_uid{};
    uid_t effective_uid{};
    gid_t real_gid{};
    gid_t effective_gid{};
    std::vector<gid_t> groups;
    bool is_root = false;
};

[[nodiscard]] Result<Identity> identity();
[[nodiscard]] Result<std::vector<std::string>> capabilities();

}  // namespace nizaw::security
