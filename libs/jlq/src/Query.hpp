#pragma once

#include "QueryConfig.hpp"

#include <cstddef>
#include <span>
#include <ostream>

namespace jlq
{

    enum class QueryStatus
    {
        Ok,
        ParseError,
    };

    // Runs the query over a memory-mapped JSONL file.
    // - In default mode: malformed/oversized lines are skipped.
    // - In strict mode: first malformed/oversized line returns QueryStatus::ParseError.
    // Writes matching lines to `out` exactly as they appear in the input.
    [[nodiscard]] QueryStatus runQuery(std::span<const std::byte> mapped,
                                       const QueryConfig &config,
                                       std::ostream &out);

} // namespace jlq
