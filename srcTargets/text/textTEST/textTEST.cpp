#include <text/text.hpp>

int main() {
    // Plain exit-code test: no framework, and it still works when NDEBUG disables assert().
    if (text::to_upper("target layout") != "TARGET LAYOUT") {
        return 1;
    }
    if (!text::to_upper("").empty()) {
        return 1;
    }
    return 0;
}
