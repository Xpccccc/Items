#pragma once

#include <iostream>

#include "utils.hpp"

namespace ns_log
{
    using namespace ns_utils;

    // 日志等级
    enum
    {
        INFO, // 就是整数
        DEBUG,
        WARNING,
        ERROR,
        FATAL
    };

    // 频繁使用，使用内联函数
    inline std::ostream &Log(const std::string &level, const std::string &filename, int line)
    {
        std::string message = "[";
        message += level + "]";
        message += "[";
        message += filename + "]";
        message += "[";
        message += std::to_string(line) + "]";
        message += "[";
        message += TimeUtil::GetCurrentTime() + "]";
        std::cout << message;
        return std::cout;
    }

    // LOG(INFO) << "message"; // 开放式日志

#define LOG(level) Log(#level, __FILE__, __LINE__)
}