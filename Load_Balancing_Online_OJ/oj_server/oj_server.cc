#include <iostream>

#include "../comm/httplib.h"
#include "oj_control.hpp"

#include <signal.h>

using namespace httplib;
using namespace ns_control;

Control *ctrl_ptr = nullptr;

void Recovery(int signo)
{
    ctrl_ptr->RecoveryMachines();
}

int main()
{
    // 用户请求的服务路由功能
    Server svr;

    Control ctrl;
    ctrl_ptr = &ctrl;
    signal(SIGQUIT, Recovery); // 一键上线功能

    // 获取题目列表
    svr.Get("/all_questions", [&ctrl](const Request &req, Response &resp)
            {   
                std::string html;
                ctrl.AllQuestions(&html);
                resp.set_content(html, "text/html;charset=utf-8"); });

    // 获取指定题目
    svr.Get(R"(/question/(\d+))", [&ctrl](const Request &req, Response &resp)
            { 
                std::string html;
                std::string number = req.matches[1];
                ctrl.OneQuestion(number,&html);
                resp.set_content(html, "text/html;charset=utf-8"); });

    // 用户提交代码，使用我们的判题功能 (1.测试用例 2.compile_run)
    // Post -- 因为要提交的东西放正文
    svr.Post(R"(/judge/(\d+))", [&ctrl](const Request &req, Response &resp)
             {
                std::string in_json = req.body;
                std::string out_json;
        std::string number = req.matches[1];
        ctrl.Judge(number,in_json,&out_json);
        resp.set_content(out_json,"applicatin/json;charset=utf-8"); });

    svr.set_base_dir("./wwwroot");

    svr.listen("0.0.0.0", 8080);

    return 0;
}