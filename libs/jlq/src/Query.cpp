#include "Query.hpp"
#include "LineScanner.hpp"

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
                ec == simdjson::NUMBER_OUT_OF_RANGE || ec == simdjson::BIGINT_ERROR ||
                ec == simdjson::INDEX_OUT_OF_BOUNDS)
            {
                return MatchResult::NoMatch;
            }

            // Everything else is treated as malformed JSON (especially in strict mode).
            return MatchResult::Malformed;
        }

        void writeBytes(std::ostream &out, std::span<const std::byte> bytes)
        {
            const auto *ptr = reinterpret_cast<const char *>(bytes.data());
            out.write(ptr, static_cast<std::streamsize>(bytes.size()));
        }

        MatchResult valueMatches(simdjson::ondemand::value value, const QueryValue &qv)
        {
            return std::visit(
                [&](auto &&arg) -> MatchResult
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::string_view>)
                    {
                        auto s = value.get_string();
                        if (s.error())
                        {
                            return classifyError(s.error());
                        }
                        return (s.value() == arg) ? MatchResult::Match : MatchResult::NoMatch;
                    }
                    else if constexpr (std::is_same_v<T, double>)
                    {
                        auto d = value.get_double();
                        if (d.error())
                        {
                            return classifyError(d.error());
                        }
                        return (d.value() == arg) ? MatchResult::Match : MatchResult::NoMatch;
                    }
                    else if constexpr (std::is_same_v<T, bool>)
                    {
                        auto b = value.get_bool();
                        if (b.error())
                        {
                            return classifyError(b.error());
                        }
                        return (b.value() == arg) ? MatchResult::Match : MatchResult::NoMatch;
                    }
                    else if constexpr (std::is_same_v<T, std::monostate>)
                    {
                        auto is_null = value.is_null();
                        if (is_null.error())
                        {
                            return classifyError(is_null.error());
                        }
                        return is_null.value() ? MatchResult::Match : MatchResult::NoMatch;
                    }
                    return MatchResult::NoMatch;
                },
                qv);
        }

        MatchResult traverseAndMatch(simdjson::ondemand::document &doc, const QueryConfig &config)
        {
            simdjson::ondemand::value current = doc;
            for (const PathSegment &seg : config.path_segments)
            {
                if (seg.kind == PathSegmentKind::Key)
                {
                    auto obj_res = current.get_object();
                    if (obj_res.error())
                    {
                        return classifyError(obj_res.error());
                    }
                    simdjson::ondemand::object obj = obj_res.value();

                    auto field_res = obj.find_field_unordered(seg.key);
                    if (field_res.error())
                    {
                        return classifyError(field_res.error());
                    }
                    current = field_res.value();
                }
                else
                {
                    auto arr_res = current.get_array();
                    if (arr_res.error())
                    {
                        return classifyError(arr_res.error());
                    }
                    simdjson::ondemand::array arr = arr_res.value();

                    auto elem_res = arr.at(seg.index);
                    if (elem_res.error())
                    {
                        return classifyError(elem_res.error());
                    }
                    current = elem_res.value();
                }
            }

            return valueMatches(current, config.value);
        }

    } // namespace

    QueryStatus runQuery(std::span<const std::byte> mapped, const QueryConfig &config, std::ostream &out)
    {
        simdjson::ondemand::parser parser;
        std::vector<char> scratch;
        scratch.reserve(LineScanner::max_line_length + simdjson::SIMDJSON_PADDING);

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
            simdjson::error_code err = simdjson::SUCCESS;
            try
            {
                err = parser.iterate(scratch.data(), json_len, scratch.size()).get(doc);
            }
            catch (const simdjson::simdjson_error &)
            {
                if (config.strict)
                {
                    return QueryStatus::ParseError;
                }
                continue;
            }

            if (err)
            {
                if (config.strict)
                {
                    return QueryStatus::ParseError;
                }
                continue;
            }

            MatchResult result = MatchResult::Malformed;
            try
            {
                result = traverseAndMatch(doc, config);
            }
            catch (const simdjson::simdjson_error &)
            {
                result = MatchResult::Malformed;
            }

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
                writeBytes(out, line.raw);
                if (line.had_newline)
                {
                    out.put('\n');
                }
            }
        }

        return QueryStatus::Ok;
    }

} // namespace jlq
