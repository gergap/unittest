#include <testlib.h>

void test_helloworld()
{
    UVERIFY2(true, "Hello World");
}

void register_tests()
{
    UREGISTER_TEST(test_helloworld);
}

UTEST_MAIN()

