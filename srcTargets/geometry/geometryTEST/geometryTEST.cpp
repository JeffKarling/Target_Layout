#include <geometry/geometry.hpp>

int main() {
    if (geometry::area({3.0, 4.0}) != 12.0) {
        return 1;
    }
    if (geometry::area({0.0, 5.0}) != 0.0) {
        return 1;
    }
    return 0;
}
