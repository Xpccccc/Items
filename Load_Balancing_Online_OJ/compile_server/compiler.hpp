#pragma once

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "../comm/utils.hpp"
#include "../comm/log.hpp"

// 只进行编译
namespace ns_compiler
{
    using namespace ns_utils;
    using namespace ns_log;
    class Compiler
    {
    public:
        Compiler() {}
        ~Compiler() {}

        // 返回值，编译成功：true，否则：false
        static bool Compile(const std::string &filename)
        {
            pid_t pid = fork(); // 子进程进行程序替换来编译这个文件
            if (pid < 0)
            {
                LOG(ERROR) << "内部错误，创建子进程失败" << "\n";
                return false;
            }
            else if (pid == 0)
            {
                // 子进程：调用编译器，进行程序替换执行文件

                umask(0); // 重置掩码

                // 创建错误信息文件，用来保存错误信息
                int err_fd = open(PathUtil::Compile_err(filename).c_str(), O_CREAT | O_WRONLY, 0644);
                // std::cout << "debug ===== " << std::endl;
                if (err_fd < 0)
                {
                    LOG(ERROR) << "内部错误，打开文件失败" << "\n";
                    exit(1);
                }

                // 重定向标准错误到errfd，以至于打印到显示器的信息，打印到Stderr文件
                dup2(err_fd, 2);

                // g++ -o 可执行文件 源文件 -std=c++11
                execlp("g++", "g++", "-o", PathUtil::Exe(filename).c_str(), PathUtil::Src(filename).c_str(), "-std=c++11", "-D", "COMPILE", nullptr /*不要忘记*/); // 注意"-D", "COMPILE"

                exit(2); // 执行完就退出
            }
            else
            {
                // 父进程
                waitpid(pid, nullptr, 0);
                // 判断可执行文件在不在，如果在，就说明编译成功，否则失败
                if (FileUtil::IsExistFile(PathUtil::Exe(filename).c_str()))
                {
                    LOG(INFO) << PathUtil::Src(filename) << " 编译成功" << "\n";

                    return true;
                }
            }
            LOG(ERROR) << "编译失败，没有形成可执行程序" << "\n";

            return false;
        }
    };
}
