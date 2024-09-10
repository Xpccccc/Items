#pragma once

#include <jsoncpp/json/json.h>

#include "compiler.hpp"
#include "runner.hpp"

#include "../comm/utils.hpp"

namespace ns_complie_run
{

    using namespace ns_compiler;
    using namespace ns_runner;
    using namespace ns_utils;
    class Compile_And_Run
    {

    public:
        static std::string StatusToDesc(int status, const std::string &filename)
        {
            std::string desc;
            switch (status)
            {
            case 0:
                desc = "编译运行成功！";
                break;
            case -1:
                desc = "代码文件为空";
                break;
            case -2:
                desc = "未知错误";
                break;
            case -3:
                FileUtil::ReadFile(PathUtil::Compile_err(filename), &desc, true); // 读取编译错误的文件内容
                break;
            case SIGABRT:
                desc = "申请空间超过限制"; // 6
                break;
            case SIGXCPU:
                desc = "CPU使用时间超过限制"; // 24
                break;
            case SIGFPE:
                desc = "浮点数溢出"; // 8
                break;
            case SIGSEGV:
                desc = "段错误, 可能是没有设置cpu时间和内存空间限制"; // 11
                break;
            default:
                desc = "未知: " + std::to_string(status);
                break;
            }
            return desc;
        }

        static void RemoveTmpFile(const std::string &filename)
        {
            std::string _src = PathUtil::Src(filename);
            if (FileUtil::IsExistFile(_src))
            {
                unlink(_src.c_str());
            }
            std::string _stdin = PathUtil::Stdin(filename);
            if (FileUtil::IsExistFile(_stdin))
            {
                unlink(_stdin.c_str());
            }
            std::string _stdout = PathUtil::Stdout(filename);
            if (FileUtil::IsExistFile(_stdout))
            {
                unlink(_stdout.c_str());
            }
            std::string _stderr = PathUtil::Stderr(filename);
            if (FileUtil::IsExistFile(_stderr))
            {
                unlink(_stderr.c_str());
            }
            std::string _compile_err = PathUtil::Compile_err(filename);
            if (FileUtil::IsExistFile(_compile_err))
            {
                unlink(_compile_err.c_str());
            }
            std::string _exe = PathUtil::Exe(filename);
            if (FileUtil::IsExistFile(_exe))
            {
                unlink(_exe.c_str());
            }
        }

    public:
        /*
            输入：
            code: 用户提交的代码
            input: 用户自己提交的用例

            in_json = {"code":"#include ....","input":"...","cpu_limit":"...","mem_limit":"..."}

            输出：
            status: 状态码
            reason: 错误原因

            下面两个选择填入
            stdout: 程序运行完的结果
            stderr: 程序异常的原因
            out_json = {"status":"...","reason":"...","stdout":"...","stderr":"..."}
        */

        // 这里的in_json是网络传输过来
        static void Compile_Run(std::string &in_json, std::string *out_json)
        {
            Json::Value in_value;
            Json::Reader reader;

            reader.parse(in_json, in_value);

            std::string code = in_value["code"].asCString();
            std::string input = in_value["input"].asCString();
            int cpu_limit = in_value["cpu_limit"].asInt();
            int mem_limit = in_value["mem_limit"].asInt();

            int status = 0;
            Json::Value out_value;
            std::string filename;
            int run_code;

            if (code.size() == 0)
            {
                // TODO
                status = -1; // 代码文件为空
                goto END;
            }

            // 形成的文件得具有唯一性
            filename = FileUtil::UniqueFileName(code);

            // 形成临时src文件
            // 把code里面的内容放到Src文件中
            if (!FileUtil::WriteFile(PathUtil::Src(filename), code))
            {
                status = -2; // 写文件失败
                goto END;
            }

            // 编译
            if (!Compiler::Compile(filename))
            {
                status = -3; // 编译失败
                goto END;
            }

            // 执行
            run_code = Runner::Run(filename, cpu_limit, mem_limit);
            status = run_code;
            if (run_code < 0)
            {
                status = -2; // 未知错误
            }
            else // run_code >= 0
            {
                // 运行成功就是0
                status = run_code;
            }

        END:
            out_value["status"] = status;
            out_value["reason"] = StatusToDesc(status, filename);
            if (status == 0)
            {
                // 整个过程全部成功
                // 运行结果在stdout文件
                std::string _stdout;
                FileUtil::ReadFile(PathUtil::Stdout(filename), &_stdout, true); // 需要\n
                out_value["stdout"] = _stdout;

                // 运行错误在stderr文件 -- 存在一点问题，运行错误不会进来这个文件
                std::string _stderr;
                FileUtil::ReadFile(PathUtil::Stderr(filename), &_stderr, true); // 需要\n
                out_value["stderr"] = _stderr;
            }

            // 序列化
            Json::StyledWriter witer;
            *out_json = witer.write(out_value);

            // 已经得到结果了，直接移除临时文件
            RemoveTmpFile(filename);
        }
    };
}