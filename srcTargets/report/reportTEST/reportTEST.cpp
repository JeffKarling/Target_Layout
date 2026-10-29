#include <report/report.hpp>

int main() {
    if (report::describe("poster", {3.0, 4.0}) != "POSTER: 3 x 4 (area 12)") {
        return 1;
    }
    return 0;
}
