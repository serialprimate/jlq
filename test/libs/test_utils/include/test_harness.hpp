#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jlq::test
{

    struct Case
    {
        std::string name;
        std::function<void()> fn;
    };

    inline std::vector<Case> &registry()
    {
        static std::vector<Case> tests;
        return tests;
    }

    struct Registrar
    {
        Registrar(std::string name, std::function<void()> fn)
        {
            registry().push_back(Case{std::move(name), std::move(fn)});
        }
    };

    [[noreturn]] inline void fail(std::string_view message, const char *file, int line)
    {
        throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " +
                                 std::string(message));
    }

    inline void check(bool ok, std::string_view expr, const char *file, int line)
    {
        if (!ok)
        {
            fail(std::string("CHECK failed: ") + std::string(expr), file, line);
        }
    }

    inline int run_all()
    {
        int failed = 0;
        for (const auto &tc : registry())
        {
            try
            {
                tc.fn();
                std::cout << "[pass] " << tc.name << "\n";
            }
            catch (const std::exception &e)
            {
                ++failed;
                std::cout << "[fail] " << tc.name << ": " << e.what() << "\n";
            }
            catch (...)
            {
                ++failed;
                std::cout << "[fail] " << tc.name << ": unknown exception\n";
            }
        }
        return failed;
    }

} // namespace jlq::test

#define JLQ_TEST_CONCAT_INNER(x, y) x##y
#define JLQ_TEST_CONCAT(x, y) JLQ_TEST_CONCAT_INNER(x, y)

#define JLQ_TEST_CASE(name_literal)                                           \
    static void JLQ_TEST_CONCAT(jlq_test_case_, __LINE__)();                  \
    static ::jlq::test::Registrar JLQ_TEST_CONCAT(jlq_test_reg_, __LINE__)(   \
        (name_literal), [] { JLQ_TEST_CONCAT(jlq_test_case_, __LINE__)        \
                             (); }); \
    static void JLQ_TEST_CONCAT(jlq_test_case_, __LINE__)()

#define JLQ_CHECK(expr) \
    ::jlq::test::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

#define JLQ_CHECK_EQ(a, b) \
    ::jlq::test::check(((a) == (b)), #a " == " #b, __FILE__, __LINE__)
