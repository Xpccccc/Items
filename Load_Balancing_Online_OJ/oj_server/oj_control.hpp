#pragma once

#include <iostream>
#include <jsoncpp/json/json.h>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <fstream>
#include <cassert>
#include <algorithm>

// #include "oj_model.hpp"
#include "oj_model_MySQL.hpp"
#include "oj_view.hpp"
#include "../comm/log.hpp"
#include "../comm/utils.hpp"
#include "../comm/httplib.h"

namespace ns_control
{
    using namespace ns_model;
    using namespace ns_view;
    using namespace ns_log;
    using namespace ns_utils;
    using namespace httplib;

    // 提供编译运行服务的主机
    class Machine
    {
    public:
        std::string _ip;
        uint16_t _port;
        uint64_t _load;   // 负载量
        std::mutex *_mtx; // 对_load进行加锁

    public:
        Machine() : _ip(""), _port(0), _load(0), _mtx(nullptr) {}

        // 增加负载
        void IncLoad()
        {
            if (_mtx)
                _mtx->lock();
            ++_load;
            if (_mtx)
                _mtx->unlock();
        }

        // 降低负载
        void DecLoad()
        {
            if (_mtx)
                _mtx->lock();
            --_load;
            if (_mtx)
                _mtx->unlock();
        }

        // 读取load，没有太大意义，只是为了统一接口
        uint64_t Load()
        {
            uint64_t load = 0;
            if (_mtx)
                _mtx->lock();
            load = _load;
            if (_mtx)
                _mtx->unlock();
            return load;
        }

        // 负载清零，一台主机下线了，它的负载得清零
        void ResetLoad()
        {
            if (_mtx)
                _mtx->lock();
            _load = 0;
            if (_mtx)
                _mtx->unlock();
        }
        ~Machine() {}
    };

    const std::string machine_conf = "./conf/machine_list.conf";

    // 提供负载均衡选择主机
    class LoadBalance
    {
    private:
        std::vector<Machine> _machines; // 提供负载均衡的主机群 -- 以下表在标识每一台主机
        std::vector<int> _online;       // 在线的主机
        std::vector<int> _offline;      // 离线的主机
        std::mutex _mtx;                // 对负载均衡选择加锁，因为可能同时到来多个用户提交代码，而_machines等是临界资源（共享的）
    public:
        LoadBalance()
        {
            assert(LoadMachine(machine_conf));
            LOG(INFO) << "加载配置文件 " << machine_conf << " 成功" << "\n";
        }

        bool LoadMachine(const std::string &machine_file)
        {
            std::ifstream in(machine_file);
            if (!in.is_open())
            {
                LOG(FATAL) << "加载配置文件 " << machine_file << " 失败" << "\n";
                return false;
            }

            std::string line;
            while (getline(in, line))
            {
                std::vector<std::string> tokens;
                StringUtil::StringSplit(line, &tokens, ":");
                if (tokens.size() != 2)
                {
                    LOG(WARNING) << "加载配置行 " << line << " 失败" << "\n";
                    continue;
                }
                Machine m;
                m._ip = tokens[0];
                m._port = std::atoi(tokens[1].c_str());
                m._load = 0;
                m._mtx = new std::mutex();
                _online.push_back(_machines.size()); // 先把上线的主机的下标对应主机 0 <-> 第一台 ...
                _machines.push_back(m);
            }
            return true;
        }

        // 智能选择负载最小的主机
        bool SmartChioce(int *id, Machine **m)
        {
            _mtx.lock();

            // 负载均衡的算法
            // 1.随机+hash
            // 2.轮询+hash
            int num_online = _online.size(); // 在线的主机
            if (num_online == 0)
            {
                _mtx.unlock(); // 出去前先解锁
                LOG(FATAL) << "没有在线的主机，赶紧检查！" << "\n";
                return false;
            }

            uint64_t min_load = _machines[_online[0]].Load();
            *id = _online[0];
            *m = &_machines[_online[0]];
            for (int i = 1; i < num_online; ++i)
            {
                uint64_t cur_load = _machines[_online[i]].Load();
                if (cur_load < min_load)
                {
                    min_load = cur_load;
                    *id = _online[i];
                    *m = &_machines[_online[i]]; // 要这台主机的地址
                }
            }

            _mtx.unlock();
            return true;
        }

        void OnlineMachine()
        {
            _mtx.lock();
            // 把离线主机添加到在线主机
            _online.insert(_online.end(), _offline.begin(), _offline.end());
            _offline.erase(_offline.begin(), _offline.end());
            _mtx.unlock();
            LOG(INFO) << "所有主机上线啦！" << "\n";
        }

        // 让当前主机进入下线状态
        void OfflineMachine(int id)
        {
            _mtx.lock();
            for (auto iter = _online.begin(); iter != _online.end(); ++iter)
            {
                if (*iter == id)
                {
                    _machines[id].ResetLoad(); // 清空当前主机的负载
                    _online.erase(iter);
                    _offline.push_back(id);
                    break; // id唯一
                }
            }
            _mtx.unlock();
        }

        void ShowMachines()
        {
            std::cout << "当前在线的主机 : ";
            for (auto id : _online)
            {
                std::cout << id << " ";
            }
            std::cout << std::endl;

            std::cout << "当前离线线的主机 : ";
            for (auto id : _offline)
            {
                std::cout << id << " ";
            }
            std::cout << std::endl;
        }

        ~LoadBalance() {}
    };

    class Control
    {
    private:
        Model _model;              // 提供后台数据
        View _view;                // 提供html渲染
        LoadBalance _load_balance; // 提供核心负载均衡器

    public:
        Control() {}

        void RecoveryMachines()
        {
            _load_balance.OnlineMachine();
        }

        bool AllQuestions(std::string *html)
        {
            bool ret = true;
            // 获取所有题目
            std::vector<Question> all;
            if (_model.GetAllQuestions(&all))
            {
                // 渲染网页
                // 排序
                sort(all.begin(), all.end(), [](const Question &q1, const Question &q2)
                     { return std::atoi(q1.number.c_str()) < std::atoi(q2.number.c_str()); });
                _view.DrawAllQuestions(all, html);
            }
            else
            {
                *html = "加载题库失败";
                ret = false;
            }
            return ret;
        }
        bool OneQuestion(const std::string &number, std::string *html)
        {
            bool ret = true;
            Question q;
            if (_model.GetOneQuestion(number, &q))
            {
                // 渲染网页
                _view.DrawOneQuestion(q, html);
            }
            else
            {
                *html = "加载题目 " + number + " 失败";
                ret = false;
            }
            return ret;
        }

        void Judge(const std::string &number, const std::string &in_json, std::string *out_json)
        {
            // 0.根据题目编号，拿到题目细节
            Question q;
            _model.GetOneQuestion(number, &q);

            // 1.对in_json进行反序列化，得到code，input
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value);
            std::string code = in_value["code"].asCString();
            std::string input = in_value["input"].asCString();

            // 2.拼接代码，header+tail
            Json::Value compile_run_value;
            compile_run_value["code"] = code + "\n" + q.tail; // 记得加\n，不然可能存在编译的时候和ifndef COMPILE 共一行
            compile_run_value["input"] = input;
            compile_run_value["cpu_limit"] = q.cpu_limit;
            compile_run_value["mem_limit"] = q.mem_limit;
            Json::StyledWriter writer;
            std::string compile_run_string = writer.write(compile_run_value);

            // 3.选择负载最低的主机
            // 规则：一直选择，直到主机可用，否则就是主机已经全部挂掉
            while (true)
            {
                int id = 0;
                Machine *m = nullptr;
                if (!_load_balance.SmartChioce(&id, &m))
                {
                    break; // 主机全部挂掉了
                }

                // 4.发起http请求，得到结果
                Client cli(m->_ip, m->_port);
                m->IncLoad(); // 用的时候增加负载值
                LOG(INFO) << "连接主机成功 , 主机id : " << id << " 详情 : " << m->_ip << ":" << m->_port << " 负载 : " << m->Load() << "\n";

                // inline Result Client::Post(const char *path, const std::string &body, const char *content_type)
                auto res = cli.Post("/compile_run", compile_run_string, "application/json;charset=utf-8");
                if (res) // 编译运行
                {
                    if (res->status == 200)
                    {
                        // 请求成功
                        // 5.将结果给out_json
                        *out_json = res->body;
                        m->DecLoad(); // 用完后减少负载值
                        break;
                    }
                    m->DecLoad();
                }
                else
                {
                    // 请求失败
                    LOG(WARNING) << "当前请求的主机id : " << id << " 详情 : " << m->_ip << ":" << m->_port << " 可能已经离线" << "\n";
                    _load_balance.OfflineMachine(id); // 当前主机下线
                    _load_balance.ShowMachines();     // 用来测试
                }
                sleep(1);
            }
        }

        ~Control() {}
    };
}