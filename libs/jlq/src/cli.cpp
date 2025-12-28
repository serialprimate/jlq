#include "jlq/cli.hpp"

#include "ExitCode.hpp"
#include "MappedFile.hpp"

#include "path.hpp"
#include "Query.hpp"
#include "QueryConfig.hpp"

#include <iostream>
#include <charconv>
#include <cmath>
#include <limits>
#include <string>

namespace jlq
{

    namespace
    {

        enum class ValueType
        {
            String,
            Number,
            Bool,
            Null,
        };

        void printUsage(std::ostream &os)
        {
            os << "Usage: jlq <file> --path <path> --value <value> [--type <type>] [--threads <n>] [--strict]\n";
            os << "\n";
            os << "Options:\n";
            os << "  --path <path>       Dot-notation path (keys + array indices, e.g. a.b.0.c)\n";
            os << "  --value <value>     Exact-match value (ignored for --type null)\n";
            os << "  --type <type>       string (default), number, bool, null\n";
            os << "  --threads <n>       Validate n >= 1 (stored; Phase 3 is single-threaded)\n";
            os << "  --strict            Malformed/oversized line => exit code 3\n";
            os << "  --help              Show this help\n";
        }

        [[nodiscard]] bool isValidJsonNumber(std::string_view s) noexcept
        {
            if (s.empty())
            {
                return false;
            }

            std::size_t i = 0;
            if (s[i] == '-')
            {
                ++i;
                if (i == s.size())
                {
                    return false;
                }
            }

            // int part: 0 | [1-9][0-9]*
            if (s[i] == '0')
            {
                ++i;
            }
            else
            {
                if (s[i] < '1' || s[i] > '9')
                {
                    return false;
                }
                ++i;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9')
                {
                    ++i;
                }
            }

            // fraction
            if (i < s.size() && s[i] == '.')
            {
                ++i;
                if (i == s.size() || s[i] < '0' || s[i] > '9')
                {
                    return false;
                }
                while (i < s.size() && s[i] >= '0' && s[i] <= '9')
                {
                    ++i;
                }
            }

            // exponent
            if (i < s.size() && (s[i] == 'e' || s[i] == 'E'))
            {
                ++i;
                if (i == s.size())
                {
                    return false;
                }
                if (s[i] == '+' || s[i] == '-')
                {
                    ++i;
                    if (i == s.size())
                    {
                        return false;
                    }
                }
                if (s[i] < '0' || s[i] > '9')
                {
                    return false;
                }
                while (i < s.size() && s[i] >= '0' && s[i] <= '9')
                {
                    ++i;
                }
            }

            return i == s.size();
        }

        [[nodiscard]] std::optional<ValueType> parseValueType(std::string_view s) noexcept
        {
            if (s == "string")
            {
                return ValueType::String;
            }
            if (s == "number")
            {
                return ValueType::Number;
            }
            if (s == "bool")
            {
                return ValueType::Bool;
            }
            if (s == "null")
            {
                return ValueType::Null;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::size_t> parseThreads(std::string_view s) noexcept
        {
            std::size_t value = 0;
            const auto *begin = s.data();
            const auto *end = s.data() + s.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end)
            {
                return std::nullopt;
            }
            if (value < 1)
            {
                return std::nullopt;
            }
            return value;
        }

        [[nodiscard]] std::optional<double> parseNumber(std::string_view s)
        {
            if (!isValidJsonNumber(s))
            {
                return std::nullopt;
            }

            // Parse as double without allocating a temporary string.
            // Grammar is already validated above, but we still require full consumption.
            double value = 0.0;
            const auto *begin = s.data();
            const auto *end = s.data() + s.size();
            const auto result = std::from_chars(begin, end, value, std::chars_format::general);
            if (result.ec != std::errc{} || result.ptr != end)
            {
                return std::nullopt;
            }
            if (!std::isfinite(value))
            {
                return std::nullopt;
            }
            return value;
        }

    } // namespace

    int run(std::span<const std::string_view> args, std::ostream &out, std::ostream &err)
    {
        // args includes argv[0]
        if (args.size() <= 1)
        {
            printUsage(err);
            return static_cast<int>(ExitCode::UsageError);
        }

        for (std::size_t i = 1; i < args.size(); ++i)
        {
            if (args[i] == "--help")
            {
                printUsage(out);
                return static_cast<int>(ExitCode::Success);
            }
        }

        const std::string_view file = args[1];
        if (file.empty() || file.starts_with('-'))
        {
            printUsage(err);
            return static_cast<int>(ExitCode::UsageError);
        }

        QueryConfig config;

        std::optional<std::string_view> path;
        std::optional<std::string_view> value;
        std::optional<std::string_view> type;
        std::optional<std::string_view> threads;

        bool strict_seen = false;
        bool path_seen = false;
        bool value_seen = false;
        bool type_seen = false;
        bool threads_seen = false;

        // Strict option parsing: only allow documented flags.
        for (std::size_t i = 2; i < args.size(); ++i)
        {
            const std::string_view a = args[i];
            if (a == "--strict")
            {
                if (strict_seen)
                {
                    printUsage(err);
                    return static_cast<int>(ExitCode::UsageError);
                }
                strict_seen = true;
                config.strict = true;
                continue;
            }

            if (a == "--path" || a == "--value" || a == "--type" || a == "--threads")
            {
                if (i + 1 >= args.size())
                {
                    printUsage(err);
                    return static_cast<int>(ExitCode::UsageError);
                }
                const std::string_view v = args[i + 1];
                ++i;

                if (a == "--path")
                {
                    if (path_seen)
                    {
                        printUsage(err);
                        return static_cast<int>(ExitCode::UsageError);
                    }
                    path_seen = true;
                    path = v;
                }
                else if (a == "--value")
                {
                    if (value_seen)
                    {
                        printUsage(err);
                        return static_cast<int>(ExitCode::UsageError);
                    }
                    value_seen = true;
                    value = v;
                }
                else if (a == "--type")
                {
                    if (type_seen)
                    {
                        printUsage(err);
                        return static_cast<int>(ExitCode::UsageError);
                    }
                    type_seen = true;
                    type = v;
                }
                else if (a == "--threads")
                {
                    if (threads_seen)
                    {
                        printUsage(err);
                        return static_cast<int>(ExitCode::UsageError);
                    }
                    threads_seen = true;
                    threads = v;
                }
                continue;
            }

            // Unknown option.
            printUsage(err);
            return static_cast<int>(ExitCode::UsageError);
        }

        if (!path.has_value())
        {
            printUsage(err);
            return static_cast<int>(ExitCode::UsageError);
        }

        try
        {
            config.path_segments = parseDotPath(*path);
        }
        catch (const std::exception &)
        {
            printUsage(err);
            return static_cast<int>(ExitCode::UsageError);
        }

        if (threads.has_value())
        {
            const auto parsed = parseThreads(*threads);
            if (!parsed.has_value())
            {
                printUsage(err);
                return static_cast<int>(ExitCode::UsageError);
            }
            config.threads = *parsed;
        }

        ValueType vt_choice = ValueType::String;
        if (type.has_value())
        {
            const auto vt = parseValueType(*type);
            if (!vt.has_value())
            {
                printUsage(err);
                return static_cast<int>(ExitCode::UsageError);
            }
            vt_choice = *vt;
        }

        if (vt_choice != ValueType::Null)
        {
            if (!value.has_value())
            {
                printUsage(err);
                return static_cast<int>(ExitCode::UsageError);
            }
        }

        switch (vt_choice)
        {
        case ValueType::String:
            config.value = value.value_or(std::string_view{});
            break;
        case ValueType::Bool:
        {
            if (!value.has_value() || (*value != "true" && *value != "false"))
            {
                printUsage(err);
                return static_cast<int>(ExitCode::UsageError);
            }
            config.value = (*value == "true");
            break;
        }
        case ValueType::Null:
            config.value = std::monostate{};
            break;
        case ValueType::Number:
        {
            if (!value.has_value())
            {
                printUsage(err);
                return static_cast<int>(ExitCode::UsageError);
            }
            const auto parsed = parseNumber(*value);
            if (!parsed.has_value())
            {
                printUsage(err);
                return static_cast<int>(ExitCode::UsageError);
            }
            config.value = *parsed;
            break;
        }
        }

        try
        {
            MappedFile mf = MappedFile::openReadonly(std::string(file));
            const QueryStatus status = runQuery(mf.bytes(), config, out);
            if (status == QueryStatus::ParseError)
            {
                return static_cast<int>(ExitCode::ParseError);
            }
        }
        catch (const std::exception &e)
        {
            err << "jlq: " << e.what() << "\n";
            return static_cast<int>(ExitCode::OsError);
        }

        return static_cast<int>(ExitCode::Success);
    }

    int run(std::span<const std::string_view> args)
    {
        return run(args, std::cout, std::cerr);
    }

} // namespace jlq
