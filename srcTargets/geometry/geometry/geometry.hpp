#pragma once

namespace geometry {

    struct Rect {
        double width;
        double height;
    };

    [[nodiscard]] double area(const Rect& rect);

} // namespace geometry
