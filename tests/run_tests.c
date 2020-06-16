#include "test.h"
#include "check_map.c"

int main()
{
    test_map();
    
    // TEST("random")
    // {
    //     TEST("sub-random")
    //     {
    //         EXPECT(1);
    //         EXPECT(2 > 5, "with message");
    //         FAIL();
    //         FAIL("failed message");
    //         PASS("with message too!");
    //     } END_TEST;

    //     TEST("loops... weeeee")
    //     {
    //         for (int i = 0; i < 10; ++i)
    //             PASS("this is it!");
    //     } END_TEST;
    // } END_TEST;

    return 0;
}
