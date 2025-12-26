#pragma once

namespace jlq
{

    enum class ExitCode : int
    {
        Success = 0,
        UsageError = 1,
        OsError = 2,
        ParseError = 3,
    };

} // namespace jlq
