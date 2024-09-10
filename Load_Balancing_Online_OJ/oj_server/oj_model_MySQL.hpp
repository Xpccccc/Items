#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

#include <cassert>
#include <cstdlib>

#include <mysql/mysql.h>

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

    const std::string questions_table = "oj_questions"; // 数据库表名
    const std::string host = "127.0.0.1";
    const uint16_t port = 3306;
    const std::string passwd = "xp1014647664";
    const std::string db = "oj";
    const std::string user = "oj_client";

    class Model
    {
    private:
        bool MySQLQuery(const std::string &sql, std::vector<Question> *out)
        {
            // 创建句柄
            MYSQL *my = mysql_init(nullptr);

            // 连接数据库
            if (nullptr == mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0))
            {
                LOG(FATAL) << "连接数据库失败!" << "\n";
                mysql_close(my); // 释放句柄
                return false;
            }

            LOG(INFO) << "连接数据库成功!" << "\n";

            mysql_set_character_set(my, "utf8");

            // 执行语句
            if (0 != mysql_query(my, sql.c_str()))
            {
                LOG(WARNING) << "执行语句 " << sql << " 失败!" << "\n";
                mysql_close(my); // 释放句柄
                return false;
            }

            // 提取结果
            MYSQL_RES *res = mysql_store_result(my);
            if (res == nullptr)
            {
                LOG(WARNING) << "查询未返回结果集" << "\n";
                mysql_close(my);
                return false;
            }

            // 获取行列数
            int rows = mysql_num_rows(res);

            // int cols = mysql_num_fields(res);

            for (int i = 0; i < rows; ++i)
            {
                // 获取每一行
                Question q;
                MYSQL_ROW row = mysql_fetch_row(res);
                q.number = row[0];
                q.title = row[1];
                q.level = row[2];
                q.cpu_limit = atoi(row[3]);
                q.mem_limit = atoi(row[4]);
                q.desc = row[5];
                q.header = row[6];
                q.tail = row[7];
                out->push_back(q);
            }

            mysql_free_result(res);
            mysql_close(my);
            return true;
        }

    public:
        Model() {}

        // 获取所有题目列表
        bool GetAllQuestions(std::vector<Question> *out)
        {
            std::string sql = "select * from ";
            sql += questions_table;
            return MySQLQuery(sql, out);
        }

        // 获取具体的一道题
        bool GetOneQuestion(const std::string &number, Question *out)
        {
            bool ret = false;
            std::string sql = "select * from ";
            sql += questions_table;
            sql += " where number=";
            sql += number;

            std::vector<Question> result;
            if (MySQLQuery(sql, &result))
            {
                if (result.size() == 1)
                {
                    *out = result[0];
                    return true;
                }
            }

            return ret;
        }

        ~Model() {}
    };
}