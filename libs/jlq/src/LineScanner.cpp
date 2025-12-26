#include "LineScanner.hpp"

#include <cstddef>

namespace jlq
{

    LineScanner::LineScanner(std::span<const std::byte> bytes) noexcept : bytes_{bytes} {}

    bool LineScanner::next(ScannedLine &out) noexcept
    {
        while (offset_ <= bytes_.size())
        {
            if (offset_ == bytes_.size())
            {
                return false;
            }

            const std::size_t line_begin = offset_;

            std::size_t line_end = line_begin;
            bool had_newline = false;

            for (; line_end < bytes_.size(); ++line_end)
            {
                if (bytes_[line_end] == static_cast<std::byte>('\n'))
                {
                    had_newline = true;
                    break;
                }
            }

            // raw excludes the '\n'
            const std::size_t raw_end = line_end;
            offset_ = had_newline ? (line_end + 1) : line_end;

            const std::size_t raw_len = raw_end - line_begin;

            // Ignore empty lines.
            if (raw_len == 0)
            {
                continue;
            }

            out = {};
            out.had_newline = had_newline;
            out.oversized = (raw_len > max_line_length);
            out.raw = bytes_.subspan(line_begin, raw_len);

            // CRLF tolerance: trim a single trailing '\r' for parsing.
            if (!out.raw.empty() && out.raw.back() == static_cast<std::byte>('\r'))
            {
                out.json = out.raw.first(out.raw.size() - 1);
                if (out.json.empty())
                {
                    // A line containing only "\r" is effectively empty.
                    continue;
                }
            }
            else
            {
                out.json = out.raw;
            }

            return true;
        }

        return false;
    }

} // namespace jlq
