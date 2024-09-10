#pragma once

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../comm/log.hpp"
#include "../comm/utils.hpp"

namespace ns_runner
{
    using namespace ns_log;
    using namespace ns_utils;
    class Runner
    {
    public:
        static void SetRlimit(int cpu_limit, int mem_limit)
        {
            // 限制cpu使用时间
            struct rlimit r_cpu;
            r_cpu.rlim_cur = cpu_limit; // 1秒
            r_cpu.rlim_max = RLIM_INFINITY;

            setrlimit(RLIMIT_CPU, &r_cpu);

            // 限制开辟空间大小
            struct rlimit r_mem;
            r_mem.rlim_cur = mem_limit * 1024; // KB
            r_mem.rlim_max = RLIM_INFINITY;

            setrlimit(RLIMIT_AS, &r_mem);
        }

    public:
        // 信号 ，0表示运行成功；-1表示内部错误，其他就是对应的信号值

        /*
            filename: 文件名，不带后缀
            cpu_limit: cpu限制使用时间（秒）
            mem_limit: 内存开辟限制（KB）
        */
        static int Run(std::string &filename, int cpu_limit, int mem_limit)
        {
            /*
                因为运行可执行文件，需要保存运行结果
                运行结果：1.执行失败 2.执行成功
                1.执行失败 把错误信息放在.stderr文件中
                2.执行成功 把执行结果放在.stdout文件中
                这里加一个可扩展的模块，比如，对于用户，可能会自己添加测试用例，那么他输入的用例需要放到.stdin文件中
            */

            // 这里执行对应的文件，我们不关心他运行后的结果，只关心它运行成功或失败

            umask(0);
            int _stdin_fd = open(PathUtil::Stdin(filename).c_str(), O_CREAT | O_WRONLY, 0644);
            int _stdout_fd = open(PathUtil::Stdout(filename).c_str(), O_CREAT | O_WRONLY, 0644);
            int _stderr_fd = open(PathUtil::Stderr(filename).c_str(), O_CREAT | O_WRONLY, 0644);

            // 必须得先保证能打开这些文件，如果打不开，后续动作将没有意义（读取不了文件，对比不了结果）
            if (_stdin_fd < 0 || _stdout_fd < 0 || _stderr_fd < 0)
            {
                LOG(FATAL) << "打开文件失败" << "\n";
                return -1; // 代表打开文件失败
            }
            pid_t pid = fork();
            if (pid < 0)
            {
                ::close(_stdin_fd);
                ::close(_stdout_fd);
                ::close(_stderr_fd);
                LOG(FATAL) << " 内部错误，创建子进程错误" << "\n";
                return -2; // 代码创建子进程失败
            }
            else if (pid == 0)
            {
                // 重定向
                dup2(_stdin_fd, 0);
                dup2(_stdout_fd, 1);
                dup2(_stderr_fd, 2);

                // 设置限制
                SetRlimit(cpu_limit, mem_limit); // 这里出问题也是使用信号

                execl(PathUtil::Exe(filename).c_str() /*文件路径*/, PathUtil::Exe(filename).c_str() /*执行文件*/, nullptr);
                exit(1);
            }
            else
            {
                ::close(_stdin_fd);
                ::close(_stdout_fd);
                ::close(_stderr_fd);
                int status = 0;
                waitpid(pid, &status, 0);
                LOG(INFO) << "运行完毕 : " << (status & 0x7f) << "\n";
                return status & 0x7f; // 信号 ，0表示运行成功；-1表示内部错误，其他就是对应的信号值
            }
            return -3; // 执行错误
        }
    };
}