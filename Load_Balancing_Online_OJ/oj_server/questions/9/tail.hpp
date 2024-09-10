#ifndef COMPILE
#include "header.hpp"
#endif

void Test1()
{
    vector<int> v = {1, 2, 3, 4, 5, 6, 8};
    int ret = Solution().Max(v);
    if (ret == 8)
    {
        std::cout << "用例通过, 用例 :{1,2,3,4,5,6,8} " << std::endl;
    }
    else
    {
        std::cout << "用例未通过, 用例 :{1,2,3,4,5,6,8}" << std::endl;
    }
}

void Test2()
{
    vector<int> v = {-1, -2, 0, -3, -4, -5, -6, -8};

    bool ret = Solution().Max(v);
    if (ret == 0)
    {
        std::cout << "用例通过, 用例 :{-1, -2, 0, -3, -4, -5, -6, -8} " << std::endl;
    }
    else
    {
        std::cout << "用例未通过, 用例 :{-1, -2, 0, -3, -4, -5, -6, -8}" << std::endl;
    }
}

int main()
{
    Test1();
    Test2();

    return 0;
}