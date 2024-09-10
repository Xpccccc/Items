#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>

void handler(int sig)
{
    std::cout << "sig : " << sig << std::endl;
    exit(-1);
}

int main()
{

    for (int i = 1; i < 32; ++i)
    {
        signal(i, handler);
    }

    // // 限制cpu使用时间
    // struct rlimit r_cpu;
    // r_cpu.rlim_cur = 1; // 1秒
    // r_cpu.rlim_max = RLIM_INFINITY;

    // setrlimit(RLIMIT_CPU, &r_cpu);

    // while (1)
    //     ;

    // 限制开辟空间大小
    struct rlimit r_mem;
    r_mem.rlim_cur = 1024 * 1024 * 40; // 40M
    r_mem.rlim_max = RLIM_INFINITY;

    setrlimit(RLIMIT_AS, &r_mem);

    int count = 0;
    while (1)
    {
        int *p = new int[1024 * 1024];
        ++count;
        std::cout << "count : " << count << std::endl;
        sleep(1);
    }
    return 0;
}