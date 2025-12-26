#include "jlq/path.hpp"

#include <stdexcept>

namespace jlq
{

    std::vector<std::string_view> parse_dot_path(std::string_view path)
    {
        if (path.empty())
        {
            throw std::invalid_argument("--path must not be empty");
        }
        if (path.front() == '.' || path.back() == '.')
        {
            throw std::invalid_argument("--path must not start or end with '.'");
        }

        std::vector<std::string_view> segments;
        std::size_t start = 0;
        while (start < path.size())
        {
            const std::size_t dot = path.find('.', start);
            const std::size_t end = (dot == std::string_view::npos) ? path.size() : dot;

            if (end == start)
            {
                throw std::invalid_argument("--path must not contain empty segments");
            }

            segments.push_back(path.substr(start, end - start));

            if (dot == std::string_view::npos)
            {
                break;
            }
            start = dot + 1;
        }

        if (segments.empty())
        {
            throw std::invalid_argument("--path must contain at least one segment");
        }

        return segments;
    }

} // namespace jlq
