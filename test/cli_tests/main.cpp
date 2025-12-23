#include "test_harness.hpp"

int main()
{
    const int failed = ::jlq::test::run_all();
    return failed == 0 ? 0 : 1;
}
