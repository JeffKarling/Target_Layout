#include <geometry/geometry.hpp>

namespace geometry {

    double area(const Rect& rect) {
        return rect.width * rect.height;
    }

} // namespace geometry
