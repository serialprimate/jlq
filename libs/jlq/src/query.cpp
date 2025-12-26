#include "jlq/query.hpp"

#include "jlq/line_scanner.hpp"

#include <simdjson.h>

#include <cstring>
#include <vector>

namespace jlq
{

    namespace
    {

        enum class MatchResult
        {
            Match,
            NoMatch,
            Malformed,
        };

        [[nodiscard]] MatchResult classifyError(simdjson::error_code ec) noexcept
        {
            if (ec == simdjson::SUCCESS)
            {
                return MatchResult::Match;
            }

            // Non-fatal for query semantics: treat as non-match.
            if (ec == simdjson::NO_SUCH_FIELD || ec == simdjson::INCORRECT_TYPE ||
                ec == simdjson::NUMBER_OUT_OF_RANGE || ec == simdjson::BIGINT_ERROR)
            {
                return MatchResult::NoMatch;
            }

            // Everything else is treated as malformed JSON (especially in strict mode).
            return MatchResult::Malformed;
        }

        void write_bytes(std::ostream &out, std::span<const std::byte> bytes)
        {
            const auto *ptr = reinterpret_cast<const char *>(bytes.data());
            out.write(ptr, static_cast<std::streamsize>(bytes.size()));
        }

        MatchResult value_matches(simdjson::ondemand::value value, const QueryValue &qv)
        {
            switch (qv.type)
            {
            case ValueType::String:
            {
                auto s = value.get_string();
                if (s.error())
                {
                    return classifyError(s.error());
                }
                return (s.value() == qv.string_value) ? MatchResult::Match : MatchResult::NoMatch;
            }
            case ValueType::Number:
            {
                auto d = value.get_double();
                if (d.error())
                {
                    return classifyError(d.error());
                }
                return (d.value() == qv.number_value) ? MatchResult::Match : MatchResult::NoMatch;
            }
            case ValueType::Bool:
            {
                auto b = value.get_bool();
                if (b.error())
                {
                    return classifyError(b.error());
                }
                return (b.value() == qv.bool_value) ? MatchResult::Match : MatchResult::NoMatch;
            }
            case ValueType::Null:
            {
                auto is_null = value.is_null();
                if (is_null.error())
                {
                    return classifyError(is_null.error());
                }
                return is_null.value() ? MatchResult::Match : MatchResult::NoMatch;
            }
            }
            return MatchResult::NoMatch;
        }

        MatchResult traverse_and_match(simdjson::ondemand::document &doc, const QueryConfig &config)
        {
            auto obj_res = doc.get_object();
            if (obj_res.error())
            {
                return classifyError(obj_res.error());
            }

            simdjson::ondemand::object obj = obj_res.value();
            for (std::size_t i = 0; i < config.path_segments.size(); ++i)
            {
                const std::string_view seg = config.path_segments[i];
                auto field_res = obj.find_field_unordered(seg);
                if (field_res.error())
                {
                    return classifyError(field_res.error());
                }

                simdjson::ondemand::value v = field_res.value();
                const bool last = (i + 1 == config.path_segments.size());
                if (last)
                {
                    return value_matches(v, config.value);
                }

                auto next_obj_res = v.get_object();
                if (next_obj_res.error())
                {
                    return classifyError(next_obj_res.error());
                }
                obj = next_obj_res.value();
            }

            return MatchResult::NoMatch;
        }

    } // namespace

    QueryStatus run_query(std::span<const std::byte> mapped, const QueryConfig &config, std::ostream &out)
    {
        simdjson::ondemand::parser parser;
        std::vector<char> scratch;

        LineScanner scanner(mapped);
        ScannedLine line;

        while (scanner.next(line))
        {
            if (line.oversized)
            {
                if (config.strict)
                {
                    return QueryStatus::ParseError;
                }
                continue;
            }

            const std::size_t json_len = line.json.size();
            scratch.resize(json_len + simdjson::SIMDJSON_PADDING);

            std::memcpy(scratch.data(), reinterpret_cast<const char *>(line.json.data()), json_len);
            std::memset(scratch.data() + json_len, 0, simdjson::SIMDJSON_PADDING);

            simdjson::ondemand::document doc;
            const auto err = parser.iterate(scratch.data(), json_len, scratch.size()).get(doc);
            if (err)
            {
                if (config.strict)
                {
                    return QueryStatus::ParseError;
                }
                continue;
            }

            const MatchResult result = traverse_and_match(doc, config);
            if (result == MatchResult::Malformed)
            {
                if (config.strict)
                {
                    return QueryStatus::ParseError;
                }
                continue;
            }

            if (result == MatchResult::Match)
            {
                write_bytes(out, line.raw);
                if (line.had_newline)
                {
                    out.put('\n');
                }
            }
        }

        return QueryStatus::Ok;
    }

} // namespace jlq
