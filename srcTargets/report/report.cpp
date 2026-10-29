#include <report/report.hpp>

#include <text/text.hpp> // PRIVATE dependency: an implementation detail, invisible to consumers.

#include <sstream>

namespace report {

    std::string describe(std::string_view name, const geometry::Rect& rect) {
        std::ostringstream out;
        out << text::to_upper(std::string{name}) << ": " << rect.width << " x " << rect.height
            << " (area " << geometry::area(rect) << ")";
        return out.str();
    }

} // namespace report
