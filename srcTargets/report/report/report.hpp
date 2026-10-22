#pragma once

#include <geometry/geometry.hpp> // PUBLIC dependency: Rect is part of this interface.

#include <string>
#include <string_view>

namespace report {

    // Example output: "POSTER: 3 x 4 (area 12)"
    [[nodiscard]] std::string describe(std::string_view name, const geometry::Rect& rect);

} // namespace report
