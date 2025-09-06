#include <gtest/gtest.h>
#include "libbi.h"


TEST(Algebra, R3Test) {
    using namespace libbi;
    R3 oa(1,2,3), a(1,2,3), b(4,5,6), c(7,8,9);
    
    // constructors
    ASSERT_EQ(R3(), R3(0,0,0));
    ASSERT_EQ(R3(1.0f,2.0f,3.0f), R3(1,2,3));

    // op=
    ASSERT_EQ(a+=b, R3(5,7,9));
    a = oa;
    ASSERT_EQ(a-=b, R3(-3,-3,-3));
    a = oa;
    ASSERT_EQ(a*=2.0f, R3(2,4,6));
    a = oa;
    ASSERT_EQ(a/=2.0f, R3(0.5f,1.0f,3.0f/2.0f));
    a = oa;

    // other ops
    ASSERT_EQ(a+b+c, R3(12,15,18));
    ASSERT_EQ(a, oa);
    ASSERT_EQ(b, R3(4,5,6));
    ASSERT_EQ(c, R3(7,8,9));

    ASSERT_EQ(a-b-c, R3(-10,-11,-12));
    ASSERT_EQ(a, oa);
    ASSERT_EQ(b, R3(4,5,6));
    ASSERT_EQ(c, R3(7,8,9));

    ASSERT_EQ(a*2.0f, R3(2,4,6));
    ASSERT_EQ(a, oa);
    ASSERT_EQ(a/2.0f, R3(0.5f,1.0f,3.0f/2.0f));
    ASSERT_EQ(a, oa);

    ASSERT_EQ(a.inner(b), 32.0f);
    ASSERT_NEAR(R3(1,0,0).angle(R3(0,1,0)), M_PI/2.0f, 1e-6f);

    ASSERT_EQ(R3(1,0,0).cross(R3(1,0,0)), R3(0,0,0));

    a.set(5.0f,6.0f,7.0f);
    ASSERT_EQ(a, R3(5,6,7));

    ASSERT_EQ(R3(2,3,4).length(), std::sqrt(29.0f));

    R3 n(20,30,40);
    n.normalize();
    ASSERT_EQ(n.length(), 1.0f);
}


