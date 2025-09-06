#include <gtest/gtest.h>
#include "libbi.h"


TEST(Algebra, R3Test) {
    using namespace libbi;
    R3 a, b(0.0f,0.0f,0.0f), c(1.0f,0.0f,0.0f), d(0.0f,0.0f,1.0f);
    ASSERT_EQ(a,b);
    ASSERT_EQ(c+d,R3(1.0f,0.0f,1.0f));
    ASSERT_EQ(c*3.0f, R3(3.0f,0.0f,0.0f));
    ASSERT_NE(a,c);
    ASSERT_NE(c,d);
}


