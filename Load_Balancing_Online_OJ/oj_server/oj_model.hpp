#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

#include <cassert>
#include <cstdlib>

#include "../comm/utils.hpp"
#include "../comm/log.hpp"

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_utils;
    struct Question
    {
        std::string number; // 题目编号 -- 唯一
        std::string title;  // 题目标题
        std::string level;  // 题目难度
        int cpu_limit;      // 时间要求
        int mem_limit;      // 空间要求

        std::string desc;   // 题目描述
        std::string header; // 题目预设给的代码
        std::string tail;   // 测试用例代码，需要和header拼接再编译运行
    };

    const std::string all_questions_path = "./questions/questions.list"; // 题目列表配置文件
    const std::string question_path = "./questions/";                    // 单个题目的路径

    class Model
    {
    private:
        // 题号：题目细节
        std::unordered_map<std::string, Question> _questions;

    public:
        Model()
        {
            assert(LoadAllQuestion(all_questions_path));
        }

        // 加载配置文件
        bool LoadAllQuestion(const std::string &question_list)
        {
            std::ifstream in(question_list);
            if (!in.is_open())
            {
                LOG(FATAL) << "加载题库失败，请检查是否有该题库" << "\n";
                return false;
            }
            std::string line;
            while (getline(in, line))
            {
                std::vector<std::string> tokens;
                StringUtil::StringSplit(line, &tokens, " ");
                if (tokens.size() != 5)
                {
                    // 1 回文数 简单 1 30000
                    // 没有读到五个数，有可能是写少了或者多了
                    LOG(WARNING) << "加载单个题目失败，请检查格式" << "\n";
                    continue;
                }
                Question q;
                q.number = tokens[0];
                q.title = tokens[1];
                q.level = tokens[2];
                q.cpu_limit = std::atoi(tokens[3].c_str());
                q.mem_limit = std::atoi(tokens[4].c_str());

                FileUtil::ReadFile(question_path + q.number + "/desc.txt", &q.desc, true);
                FileUtil::ReadFile(question_path + q.number + "/header.hpp", &q.header, true);
                FileUtil::ReadFile(question_path + q.number + "/tail.hpp", &q.tail, true);

                // std::cout << "path : " << (question_path + q.number + "/desc.txt") << " desc : " << q.desc << std::endl;
                // std::cout << "header : " << q.header << std::endl;
                // std::cout << "tail : " << q.tail << std::endl;

                _questions.insert({q.number, q});
            }
            LOG(INFO) << "加载题库成功！" << "\n";
            in.close();
            return true;
        }

        // 获取所有题目列表
        bool GetAllQuestions(std::vector<Question> *out)
        {
            if (_questions.size() == 0)
            {
                LOG(ERROR) << "用户获取题库失败" << "\n";
                return false;
            }
            for (const auto &q : _questions)
            {
                out->push_back(q.second);
            }
            return true;
        }

        // 获取具体的一道题
        bool GetOneQuestion(const std::string &number, Question *out)
        {
            auto iter = _questions.find(number);
            if (iter == _questions.end())
            {
                LOG(ERROR) << "用户获取题目失败，题目编号：" << number << "\n";
                return false;
            }

            *out = iter->second;
            return true;
        }

        ~Model() {}
    };
}