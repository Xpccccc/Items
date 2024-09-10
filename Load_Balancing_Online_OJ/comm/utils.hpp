#pragma once

#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>
#include <sys/time.h>
#include <boost/algorithm/string.hpp>

namespace ns_utils
{
    const std::string tmp_path = "./tmp/";

    class TimeUtil
    {
    public:
        // 秒级时间戳
        static std::string GetTimeStamp()
        {
            struct timeval t;
            gettimeofday(&t, nullptr);
            return std::to_string(t.tv_sec);
        }

        // 毫秒级时间戳
        static std::string GetMicroTimeStamp()
        {
            struct timeval t;
            gettimeofday(&t, nullptr);
            return std::to_string(t.tv_sec * 1000 + t.tv_usec / 1000);
        }

        // 获取当前时间
        static std::string GetCurrentTime()
        {
            // int gettimeofday(struct timeval *tv, struct timezone *tz);
            time_t curr_time = time(nullptr);
            struct tm *format_time = localtime(&curr_time);
            if (format_time == nullptr)
                return "None";
            char buff[1024];
            snprintf(buff, sizeof(buff), "%d-%d-%d %d:%d:%d",
                     format_time->tm_year + 1900,
                     format_time->tm_mon + 1,
                     format_time->tm_mday,
                     format_time->tm_hour,
                     format_time->tm_min,
                     format_time->tm_sec);
            return buff;
        }
    };

    class PathUtil
    {

        static std::string AddSuffix(const std::string &filename, const std::string &suffix)
        {
            std::string pathname = tmp_path;
            pathname += filename;
            pathname += suffix;
            return pathname;
        }

    public:
        // 给文件加后缀

        // 生成源文件
        static std::string Src(const std::string &filename)
        {
            return AddSuffix(filename, ".cpp");
        }
        // 生成可执行文件
        static std::string Exe(const std::string &filename)
        {
            return AddSuffix(filename, ".exe");
        }

        // 生成标准输入文件
        static std::string Stdin(const std::string &filename)
        {
            return AddSuffix(filename, ".stdin");
        }

        // 生成标准输出文件
        static std::string Stdout(const std::string &filename)
        {
            return AddSuffix(filename, ".stdout");
        }

        // 生成标准错误文件  -- 运行时错误
        static std::string Stderr(const std::string &filename)
        {
            return AddSuffix(filename, ".stderr");
        }

        // 生成编译错误文件
        static std::string Compile_err(const std::string &filename)
        {
            return AddSuffix(filename, ".compile_err");
        }
    };

    class FileUtil
    {
    public:
        // 毫秒级时间戳 + 原子性++ = 唯一文件名
        static std::string UniqueFileName(const std::string &filename)
        {
            static std::atomic_int id(0);
            ++id;
            std::string sec = TimeUtil::GetMicroTimeStamp();
            return sec + "_" + std::to_string(id);
        }

        static bool WriteFile(const std::string &filename, const std::string &content)
        {
            std::ofstream out(filename);
            if (!out.is_open())
            {
                return false;
            }

            out.write(content.c_str(), content.size());
            out.close();
            return true;
        }

        // keep -- 是否要给每一行加上\n，因为getline取每一行会去除\n
        static bool ReadFile(const std::string &filename, std::string *content, bool keep = false)
        {
            std::ifstream in(filename);
            if (!in.is_open())
            {
                return false;
            }
            std::string line;
            while (getline(in, line))
            {
                (*content) += line;
                (*content) += (keep ? "\n" : "");
            }
            in.close();
            return true;
        }

        static bool IsExistFile(const std::string &pathname)
        {
            struct stat st;
            if (stat(pathname.c_str(), &st) == 0)
            {
                // 文件存在
                return true;
            }
            return false;
        }
    };

    class StringUtil
    {
    public:
        static void StringSplit(const std::string &str, std::vector<std::string> *out, const std::string &sep)
        {
            boost::split(*out, str, boost::is_any_of(sep), boost::algorithm::token_compress_on);
        }
    };

}