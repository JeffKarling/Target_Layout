#include <report/report.hpp>

#include <iostream>

int main() {
    std::cout << report::describe("poster", {3.0, 4.0}) << '\n';
    return 0;
}
