#include "temp_file.hpp"
#include "test_harness.hpp"

#include "jlq/mapped_file.hpp"

#include <cstddef>
#include <string>

JLQ_TEST_CASE("MappedFile maps and exposes bytes")
{
    jlq::test::TempFile tmp("jlq_test_", ".txt");
    tmp.writeAll("hello\nworld\n");

    jlq::MappedFile mf = jlq::MappedFile::openReadonly(tmp.path().string());
    JLQ_CHECK_EQ(mf.size(), static_cast<std::size_t>(12));
    JLQ_CHECK(!mf.empty());

    const auto bytes = mf.bytes();
    JLQ_CHECK_EQ(bytes.size(), mf.size());

    std::string roundtrip;
    roundtrip.resize(bytes.size());
    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        roundtrip[i] = static_cast<char>(bytes[i]);
    }
    JLQ_CHECK_EQ(roundtrip, std::string("hello\nworld\n"));
}

JLQ_TEST_CASE("MappedFile supports empty files")
{
    jlq::test::TempFile tmp("jlq_test_", ".txt");
    tmp.writeAll("");

    jlq::MappedFile mf = jlq::MappedFile::openReadonly(tmp.path().string());
    JLQ_CHECK_EQ(mf.size(), static_cast<std::size_t>(0));
    JLQ_CHECK(mf.empty());
    JLQ_CHECK_EQ(mf.bytes().size(), static_cast<std::size_t>(0));
}
