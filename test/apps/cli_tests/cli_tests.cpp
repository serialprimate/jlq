#include "TempFile.hpp"
#include "test_harness.hpp"
#include "jlq/cli.hpp"
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    struct RunResult
    {
        int rc{0};
        std::string out;
        std::string err;
    };

    RunResult runArgs(std::initializer_list<std::string> args)
    {
        std::vector<std::string> owned(args);
        std::vector<std::string_view> views;
        views.reserve(owned.size());
        for (const auto &s : owned)
        {
            views.emplace_back(s);
        }

        std::ostringstream out;
        std::ostringstream err;
        const int rc = jlq::run(std::span<const std::string_view>(views), out, err);
        return RunResult{rc, out.str(), err.str()};
    }
} // namespace

JLQ_TEST_CASE("CLI returns usage error on missing args")
{
    const auto r = runArgs({"jlq"});
    JLQ_CHECK_EQ(r.rc, 1);
}

JLQ_TEST_CASE("CLI --help returns success")
{
    const auto r = runArgs({"jlq", "--help"});
    JLQ_CHECK_EQ(r.rc, 0);
}

JLQ_TEST_CASE("CLI returns usage error on unknown option")
{
    const auto r = runArgs({"jlq", "file.jsonl", "--nope"});
    JLQ_CHECK_EQ(r.rc, 1);
}

JLQ_TEST_CASE("CLI returns usage error when required flags missing")
{
    jlq::test::TempFile tmp("jlq_cli_test_", ".jsonl");
    tmp.writeAll("{\"a\":1}\n");

    const auto r = runArgs({"jlq", tmp.path().string()});
    JLQ_CHECK_EQ(r.rc, 1);
}

JLQ_TEST_CASE("CLI returns OS error when file missing")
{
    const auto r = runArgs({
        "jlq",
        "/definitely/does/not/exist.jsonl",
        "--path",
        "a",
        "--value",
        "x",
    });
    JLQ_CHECK_EQ(r.rc, 2);
}

JLQ_TEST_CASE("CLI validates bool values")
{
    jlq::test::TempFile tmp("jlq_cli_test_", ".jsonl");
    tmp.writeAll("{\"a\":{\"b\":true}}\n");

    const auto bad = runArgs({
        "jlq",
        tmp.path().string(),
        "--path",
        "a.b",
        "--type",
        "bool",
        "--value",
        "TRUE",
    });
    JLQ_CHECK_EQ(bad.rc, 1);

    const auto ok = runArgs({
        "jlq",
        tmp.path().string(),
        "--path",
        "a.b",
        "--type",
        "bool",
        "--value",
        "true",
    });
    JLQ_CHECK_EQ(ok.rc, 0);
    JLQ_CHECK_EQ(ok.out, std::string("{\"a\":{\"b\":true}}\n"));
}

JLQ_TEST_CASE("CLI default mode skips malformed lines")
{
    jlq::test::TempFile tmp("jlq_cli_test_", ".jsonl");
    tmp.writeAll("{bad}\n{\"a\":{\"b\":\"x\"}}\n");

    const auto r = runArgs({
        "jlq",
        tmp.path().string(),
        "--path",
        "a.b",
        "--value",
        "x",
    });
    JLQ_CHECK_EQ(r.rc, 0);
    JLQ_CHECK_EQ(r.out, std::string("{\"a\":{\"b\":\"x\"}}\n"));
}

JLQ_TEST_CASE("CLI strict mode stops on malformed lines")
{
    jlq::test::TempFile tmp("jlq_cli_test_", ".jsonl");
    tmp.writeAll("{\"a\":\n{\"a\":{\"b\":\"x\"}}\n");

    const auto r = runArgs({
        "jlq",
        tmp.path().string(),
        "--strict",
        "--path",
        "a.b",
        "--value",
        "x",
    });
    JLQ_CHECK_EQ(r.rc, 3);
    JLQ_CHECK_EQ(r.out, std::string(""));
}

JLQ_TEST_CASE("CLI preserves final-line newline behavior")
{
    jlq::test::TempFile tmp("jlq_cli_test_", ".jsonl");
    tmp.writeAll("{\"a\":{\"b\":\"x\"}}");

    const auto r = runArgs({
        "jlq",
        tmp.path().string(),
        "--path",
        "a.b",
        "--value",
        "x",
    });
    JLQ_CHECK_EQ(r.rc, 0);
    JLQ_CHECK_EQ(r.out, std::string("{\"a\":{\"b\":\"x\"}}"));
}
