#ifndef COMPILE
#include "header.hpp"
#endif

void Test1()
{
    bool ret = Solution().IsPalindrome(121);
    if (ret)
    {
        std::cout << "用例通过, 用例 :121 " << std::endl;
    }
    else
    {
        std::cout << "用例未通过, 用例 :121" << std::endl;
    }
}

void Test2()
{
    bool ret = Solution().IsPalindrome(12);
    if (!ret)
    {
        std::cout << "用例通过, 用例 :12 " << std::endl;
    }
    else
    {
        std::cout << "用例未通过, 用例 :12" << std::endl;
    }
}

int main()
{
    Test1();
    Test2();

    return 0;
}