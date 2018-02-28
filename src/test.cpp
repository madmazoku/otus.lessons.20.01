#define BOOST_TEST_MODULE TestMain
#include <boost/test/unit_test.hpp>

#include "../bin/version.h"

#include <thread>
#include <algorithm> 

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_version )
{
    BOOST_CHECK_GT(build_version(), 0);
}

void eat_cpu(size_t power) {
    std::vector<size_t> v(power);
    for(size_t p = 0; p < power; ++p)
        v[p] = p;
    while(std::next_permutation(v.begin(), v.end()))
        ;
}

BOOST_AUTO_TEST_CASE( test_threads )
{
    std::thread t1([](){eat_cpu(12);});
    std::thread t2([](){eat_cpu(12);});
    std::thread t3([](){eat_cpu(12);});
    std::thread t4([](){eat_cpu(12);});
    std::thread t5([](){eat_cpu(12);});
    std::thread t6([](){eat_cpu(12);});
    std::thread t7([](){eat_cpu(12);});
    std::thread t8([](){eat_cpu(12);});

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
}

/*
1: 100%
real    1m1.627s
user    1m1.572s
sys 0m0.012s
u/r: 1

2: 200%
real    1m3.647s
user    2m6.976s
sys 0m0.004s
u/r: 2

3: 291%-299%
real    1m25.785s
user    4m3.892s
sys 0m0.156s
u/r: 2.85

4: 380%
real    1m53.493s
user    6m10.408s
sys 0m1.068s
u/r: 3.27

5: 376%-379%
real    2m8.023s
user    7m48.308s
sys 0m0.308s
u/r: 3.65

6: 376%-379%
real    2m30.536s
user    9m23.404s
sys 0m0.356s
u/r: 3.75

7: 370%-376%
real    2m54.279s
user    10m58.924s
sys 0m0.460s
u/r: 3.78

8: 380%
real    3m22.799s
user    12m32.328s
sys 0m0.488s
u/r: 3.72

*/

BOOST_AUTO_TEST_SUITE_END()

