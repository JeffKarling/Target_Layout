#pragma once

#include <string>

namespace text {

    // Returns an upper-cased copy of the input (ASCII only).
    [[nodiscard]] std::string to_upper(std::string value);

} // namespace text
