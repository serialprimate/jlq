#include "temp_file.hpp"
#include "test_harness.hpp"

#include "jlq/cli.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace
{

    int runArgs(std::initializer_list<std::string_view> args)
    {
        std::vector<std::string_view> v(args);
        return jlq::run(std::span<const std::string_view>(v));
    }

} // namespace

JLQ_TEST_CASE("CLI returns usage error on missing args")
{
    const int rc = runArgs({"jlq"});
    JLQ_CHECK_EQ(rc, 1);
}

JLQ_TEST_CASE("CLI --help returns success")
{
    const int rc = runArgs({"jlq", "--help"});
    JLQ_CHECK_EQ(rc, 0);
}

JLQ_TEST_CASE("CLI returns usage error on extra args")
{
    const int rc = runArgs({"jlq", "file.jsonl", "--path"});
    JLQ_CHECK_EQ(rc, 1);
}

JLQ_TEST_CASE("CLI returns OS error when file missing")
{
    const int rc = runArgs({"jlq", "/definitely/does/not/exist.jsonl"});
    JLQ_CHECK_EQ(rc, 2);
}

JLQ_TEST_CASE("CLI succeeds on readable file")
{
    jlq::test::TempFile tmp("jlq_cli_test_", ".jsonl");
    tmp.writeAll("{\"a\":1}\n");
    const int rc = runArgs({"jlq", tmp.path().string()});
    JLQ_CHECK_EQ(rc, 0);
}

JLQ_TEST_CASE("CLI succeeds on empty file")
{
    jlq::test::TempFile tmp("jlq_cli_test_", ".jsonl");
    tmp.writeAll("");
    const int rc = runArgs({"jlq", tmp.path().string()});
    JLQ_CHECK_EQ(rc, 0);
}
