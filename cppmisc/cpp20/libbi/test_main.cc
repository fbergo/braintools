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

TEST(Algebra, T3Test) {
    using namespace libbi;

    T3 z,i;
    z.zero();
    i.identity();
    ASSERT_EQ(z.apply(R3(1,1,1)), R3(0,0,0));
    ASSERT_EQ(z.apply(R3(5,6,7)), R3(0,0,0));
    ASSERT_EQ(i.apply(R3(1,1,1)), R3(1,1,1));
    ASSERT_EQ(i.apply(R3(5,6,7)), R3(5,6,7));

    T3 x1,x2,y1,y2,z1,z2,s1,s2,s3,s4;
    x1.xrot(10.0f);
    x2.xrot(-10.0f);
    y1.yrot(10.0f);
    y2.yrot(-10.0f);
    z1.zrot(10.0f);
    z2.zrot(-10.0f);
    s1.scale(3.0f,4.0f,5.0f);
    s2.scale(1.0f/3.0f,1.0f/4.0f,1.0f/5.0f);
    s3.scale(99.0f);
    s4.scale(1.0f/99.0f);
    R3 o,p(9,8,7);

    ASSERT_NEAR((x1.apply(x2.apply(p))-p).length(), 0.0f, 1.0e-5f);
    ASSERT_NEAR((y1.apply(y2.apply(p))-p).length(), 0.0f, 1.0e-5f);
    ASSERT_NEAR((z1.apply(z2.apply(p))-p).length(), 0.0f, 1.0e-5f);
    ASSERT_NEAR((s1.apply(s2.apply(p))-p).length(), 0.0f, 1.0e-5f);
    ASSERT_NEAR((s3.apply(s4.apply(p))-p).length(), 0.0f, 1.0e-5f);

    T3 c1 = s3*x1*y1*z1*s1;
    T3 c2 = s2*z2*y2*x2*s4;
    ASSERT_NEAR((c1.apply(c2.apply(p))-p).length(), 0.0f, 1.0e-5f);

    T3 id;
    id.identity();
    ASSERT_NEAR((id.apply(p)-p).length(), 0.0f, 1.0e-5f);

}
