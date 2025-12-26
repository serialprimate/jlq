#include "test_harness.hpp"

#include "jlq/line_scanner.hpp"
#include "jlq/path.hpp"
#include "jlq/query.hpp"

#include <cstddef>
#include <cstring>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    [[nodiscard]] std::span<const std::byte> asBytes(const std::string &s)
    {
        return {reinterpret_cast<const std::byte *>(s.data()), s.size()};
    }

    [[nodiscard]] std::string_view asStringView(const std::vector<std::string_view> &segments, std::size_t i)
    {
        return segments.at(i);
    }
} // namespace

JLQ_TEST_CASE("parse_dot_path rejects invalid forms")
{
    JLQ_CHECK([]
              {
        try
        {
            (void)jlq::parse_dot_path("");
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        } }());

    JLQ_CHECK([]
              {
        try
        {
            (void)jlq::parse_dot_path(".a");
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        } }());

    JLQ_CHECK([]
              {
        try
        {
            (void)jlq::parse_dot_path("a.");
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        } }());

    JLQ_CHECK([]
              {
        try
        {
            (void)jlq::parse_dot_path("a..b");
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        } }());
}

JLQ_TEST_CASE("parse_dot_path splits segments")
{
    const auto segs = jlq::parse_dot_path("a.b.c");
    JLQ_CHECK_EQ(segs.size(), static_cast<std::size_t>(3));
    JLQ_CHECK_EQ(asStringView(segs, 0), std::string_view("a"));
    JLQ_CHECK_EQ(asStringView(segs, 1), std::string_view("b"));
    JLQ_CHECK_EQ(asStringView(segs, 2), std::string_view("c"));
}

JLQ_TEST_CASE("LineScanner splits on newline and preserves last line without newline")
{
    const std::string input = "one\ntwo";
    jlq::LineScanner scanner(asBytes(input));
    jlq::ScannedLine line;

    JLQ_CHECK(scanner.next(line));
    JLQ_CHECK_EQ(std::string(reinterpret_cast<const char *>(line.raw.data()), line.raw.size()), std::string("one"));
    JLQ_CHECK(line.had_newline);
    JLQ_CHECK(!line.oversized);

    JLQ_CHECK(scanner.next(line));
    JLQ_CHECK_EQ(std::string(reinterpret_cast<const char *>(line.raw.data()), line.raw.size()), std::string("two"));
    JLQ_CHECK(!line.had_newline);
    JLQ_CHECK(!line.oversized);

    JLQ_CHECK(!scanner.next(line));
}

JLQ_TEST_CASE("LineScanner ignores empty lines and treats CR-only line as empty")
{
    const std::string input = "\n\r\nX\n";
    jlq::LineScanner scanner(asBytes(input));
    jlq::ScannedLine line;

    JLQ_CHECK(scanner.next(line));
    JLQ_CHECK_EQ(std::string(reinterpret_cast<const char *>(line.raw.data()), line.raw.size()), std::string("X"));
    JLQ_CHECK(line.had_newline);
    JLQ_CHECK(!scanner.next(line));
}

JLQ_TEST_CASE("LineScanner trims a single trailing CR for parsing")
{
    const std::string input = "{}\r\n";
    jlq::LineScanner scanner(asBytes(input));
    jlq::ScannedLine line;

    JLQ_CHECK(scanner.next(line));
    JLQ_CHECK_EQ(line.raw.size(), static_cast<std::size_t>(3));
    JLQ_CHECK_EQ(line.json.size(), static_cast<std::size_t>(2));
    JLQ_CHECK(line.had_newline);
}

JLQ_TEST_CASE("run_query matches strings and preserves CRLF bytes")
{
    const std::string input = "{\"a\":{\"b\":\"x\"}}\r\n{\"a\":{\"b\":\"y\"}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parse_dot_path("a.b");
    cfg.value.type = jlq::ValueType::String;
    cfg.value.string_value = "x";

    std::ostringstream out;
    const auto status = jlq::run_query(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
    JLQ_CHECK_EQ(out.str(), std::string("{\"a\":{\"b\":\"x\"}}\r\n"));
}

JLQ_TEST_CASE("run_query matches null without requiring a value")
{
    const std::string input = "{\"a\":{\"b\":null}}\n{\"a\":{\"b\":\"x\"}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parse_dot_path("a.b");
    cfg.value.type = jlq::ValueType::Null;

    std::ostringstream out;
    const auto status = jlq::run_query(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
    JLQ_CHECK_EQ(out.str(), std::string("{\"a\":{\"b\":null}}\n"));
}

JLQ_TEST_CASE("run_query matches numbers by numeric equality")
{
    const std::string input = "{\"a\":{\"n\":1}}\n{\"a\":{\"n\":2}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parse_dot_path("a.n");
    cfg.value.type = jlq::ValueType::Number;
    cfg.value.number_value = 2.0;

    std::ostringstream out;
    const auto status = jlq::run_query(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
    JLQ_CHECK_EQ(out.str(), std::string("{\"a\":{\"n\":2}}\n"));
}

JLQ_TEST_CASE("run_query strict mode fails fast on malformed JSON")
{
    const std::string input = "{\"a\":\n{\"a\":{\"b\":\"x\"}}\n";

    jlq::QueryConfig cfg;
    cfg.strict = true;
    cfg.path_segments = jlq::parse_dot_path("a.b");
    cfg.value.type = jlq::ValueType::String;
    cfg.value.string_value = "x";

    std::ostringstream out;
    const auto status = jlq::run_query(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::ParseError);
    JLQ_CHECK_EQ(out.str(), std::string(""));
}

JLQ_TEST_CASE("run_query oversize line is skipped by default and errors in strict")
{
    std::string big;
    big.resize(jlq::LineScanner::max_line_length + 1, 'a');

    const std::string input = big + "\n" + "{\"a\":{\"b\":\"x\"}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parse_dot_path("a.b");
    cfg.value.type = jlq::ValueType::String;
    cfg.value.string_value = "x";

    {
        std::ostringstream out;
        const auto status = jlq::run_query(asBytes(input), cfg, out);
        JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
        JLQ_CHECK_EQ(out.str(), std::string("{\"a\":{\"b\":\"x\"}}\n"));
    }

    cfg.strict = true;
    {
        std::ostringstream out;
        const auto status = jlq::run_query(asBytes(input), cfg, out);
        JLQ_CHECK_EQ(status, jlq::QueryStatus::ParseError);
    }
}
