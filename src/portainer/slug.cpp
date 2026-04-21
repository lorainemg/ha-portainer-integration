#include "slug.h"
#include <cctype>

std::string slugify(const std::string& name) {
    std::string slug;
    auto separator = false;
    for (const auto c : name) {
        if (auto intChar = static_cast<unsigned char>(c); isalnum(intChar)) {
            if (separator) slug += '_';
            slug += static_cast<char>(std::tolower(intChar));
            separator = false;
        }
        else if (!separator && !slug.empty())
            separator = true;
    }
    return slug;
}
