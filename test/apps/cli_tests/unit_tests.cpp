#include "test_harness.hpp"

#include "LineScanner.hpp"
#include "path.hpp"
#include "Query.hpp"

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
} // namespace

JLQ_TEST_CASE("parseDotPath rejects invalid forms")
{
    JLQ_CHECK([]
              {
        try
        {
            (void)jlq::parseDotPath("");
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
            (void)jlq::parseDotPath(".a");
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
            (void)jlq::parseDotPath("a.");
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
            (void)jlq::parseDotPath("a..b");
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        } }());
}

JLQ_TEST_CASE("parseDotPath splits segments")
{
    const auto segs = jlq::parseDotPath("a.b.c");
    JLQ_CHECK_EQ(segs.size(), static_cast<std::size_t>(3));
    JLQ_CHECK_EQ(segs.at(0).kind, jlq::PathSegmentKind::Key);
    JLQ_CHECK_EQ(segs.at(0).key, std::string_view("a"));
    JLQ_CHECK_EQ(segs.at(1).kind, jlq::PathSegmentKind::Key);
    JLQ_CHECK_EQ(segs.at(1).key, std::string_view("b"));
    JLQ_CHECK_EQ(segs.at(2).kind, jlq::PathSegmentKind::Key);
    JLQ_CHECK_EQ(segs.at(2).key, std::string_view("c"));
}

JLQ_TEST_CASE("parseDotPath parses numeric segments as indices")
{
    const auto segs = jlq::parseDotPath("a.b.0.c");
    JLQ_CHECK_EQ(segs.size(), static_cast<std::size_t>(4));

    JLQ_CHECK_EQ(segs.at(0).kind, jlq::PathSegmentKind::Key);
    JLQ_CHECK_EQ(segs.at(0).key, std::string_view("a"));

    JLQ_CHECK_EQ(segs.at(1).kind, jlq::PathSegmentKind::Key);
    JLQ_CHECK_EQ(segs.at(1).key, std::string_view("b"));

    JLQ_CHECK_EQ(segs.at(2).kind, jlq::PathSegmentKind::Index);
    JLQ_CHECK_EQ(segs.at(2).index, static_cast<std::size_t>(0));

    JLQ_CHECK_EQ(segs.at(3).kind, jlq::PathSegmentKind::Key);
    JLQ_CHECK_EQ(segs.at(3).key, std::string_view("c"));
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

JLQ_TEST_CASE("runQuery matches strings and preserves CRLF bytes")
{
    const std::string input = "{\"a\":{\"b\":\"x\"}}\r\n{\"a\":{\"b\":\"y\"}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parseDotPath("a.b");
    cfg.value = std::string_view("x");

    std::ostringstream out;
    const auto status = jlq::runQuery(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
    JLQ_CHECK_EQ(out.str(), std::string("{\"a\":{\"b\":\"x\"}}\r\n"));
}

JLQ_TEST_CASE("runQuery supports array indexing in paths")
{
    const std::string input = "{\"a\":{\"b\":[{\"c\":\"x\"},{\"c\":\"y\"}]}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parseDotPath("a.b.1.c");
    cfg.value = std::string_view("y");

    std::ostringstream out;
    const auto status = jlq::runQuery(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
    JLQ_CHECK_EQ(out.str(), input);
}

JLQ_TEST_CASE("runQuery treats out-of-bounds array index as non-match")
{
    const std::string input = "{\"a\":{\"b\":[{\"c\":\"x\"}]}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parseDotPath("a.b.5.c");
    cfg.value = std::string_view("x");

    std::ostringstream out;
    const auto status = jlq::runQuery(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
    JLQ_CHECK_EQ(out.str(), std::string(""));
}

JLQ_TEST_CASE("runQuery matches null without requiring a value")
{
    const std::string input = "{\"a\":{\"b\":null}}\n{\"a\":{\"b\":\"x\"}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parseDotPath("a.b");
    cfg.value = std::monostate{};

    std::ostringstream out;
    const auto status = jlq::runQuery(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
    JLQ_CHECK_EQ(out.str(), std::string("{\"a\":{\"b\":null}}\n"));
}

JLQ_TEST_CASE("runQuery matches numbers by numeric equality")
{
    const std::string input = "{\"a\":{\"n\":1}}\n{\"a\":{\"n\":2}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parseDotPath("a.n");
    cfg.value = 2.0;

    std::ostringstream out;
    const auto status = jlq::runQuery(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
    JLQ_CHECK_EQ(out.str(), std::string("{\"a\":{\"n\":2}}\n"));
}

JLQ_TEST_CASE("runQuery strict mode fails fast on malformed JSON")
{
    const std::string input = "{\"a\":\n{\"a\":{\"b\":\"x\"}}\n";

    jlq::QueryConfig cfg;
    cfg.strict = true;
    cfg.path_segments = jlq::parseDotPath("a.b");
    cfg.value = std::string_view("x");

    std::ostringstream out;
    const auto status = jlq::runQuery(asBytes(input), cfg, out);
    JLQ_CHECK_EQ(status, jlq::QueryStatus::ParseError);
    JLQ_CHECK_EQ(out.str(), std::string(""));
}

JLQ_TEST_CASE("runQuery oversize line is skipped by default and errors in strict")
{
    std::string big;
    big.resize(jlq::LineScanner::max_line_length + 1, 'a');

    const std::string input = big + "\n" + "{\"a\":{\"b\":\"x\"}}\n";

    jlq::QueryConfig cfg;
    cfg.path_segments = jlq::parseDotPath("a.b");
    cfg.value = std::string_view("x");

    {
        std::ostringstream out;
        const auto status = jlq::runQuery(asBytes(input), cfg, out);
        JLQ_CHECK_EQ(status, jlq::QueryStatus::Ok);
        JLQ_CHECK_EQ(out.str(), std::string("{\"a\":{\"b\":\"x\"}}\n"));
    }

    cfg.strict = true;
    {
        std::ostringstream out;
        const auto status = jlq::runQuery(asBytes(input), cfg, out);
        JLQ_CHECK_EQ(status, jlq::QueryStatus::ParseError);
    }
}
