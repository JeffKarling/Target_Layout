#include <text/text.hpp>

#include <algorithm>
#include <cctype>

namespace text {

    std::string to_upper(std::string value) {
        std::ranges::transform(value, value.begin(),
                               [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return value;
    }

} // namespace text
