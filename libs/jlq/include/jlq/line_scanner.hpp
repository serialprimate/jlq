#pragma once

#include <cstddef>
#include <span>

namespace jlq
{

    struct ScannedLine
    {
        // The original bytes for this line, excluding the '\n' delimiter.
        // If the input line ended with '\n', `had_newline` is true and the caller
        // can write '\n' if it wants to preserve it.
        std::span<const std::byte> raw{};

        // Bytes to parse as JSON. This is `raw` possibly with a single trailing '\r'
        // removed (CRLF tolerance).
        std::span<const std::byte> json{};

        bool had_newline{false};
        bool oversized{false};
    };

    class LineScanner
    {
    public:
        static constexpr std::size_t max_line_length = 64ULL * 1024ULL * 1024ULL;

        explicit LineScanner(std::span<const std::byte> bytes) noexcept;

        // Advances to the next non-empty line.
        // Returns false when there are no more lines.
        [[nodiscard]] bool next(ScannedLine &out) noexcept;

    private:
        std::span<const std::byte> bytes_{};
        std::size_t offset_{0};
    };

} // namespace jlq
