#include <jsoncpp/json/json.h>

#include "../comm/httplib.h"
#include "compile_run.hpp"

using namespace ns_complie_run;
using namespace httplib;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage : \n        ./compile_server port" << std::endl;
        return 1;
    }

    uint16_t serverport = std::stoi(argv[1]);

    // 把compile_run打包成网络服务
    Server svr;
    svr.Get("/hello", [](const Request &req, Response &resp)
            { resp.set_content("hello httplib,你好啊", "text/plain;charset=utf-8"); });

    svr.Post("/compile_run", [](const Request &req, Response &resp)
             {
        std::string in_json = req.body;
        std::string out_json;

        if (!in_json.empty())
        {
            Compile_And_Run::Compile_Run(in_json, &out_json);
            resp.set_content(out_json,"application/json;charset=utf-8");
        } });

    // 绑定特定端口，多次启动可以绑定多个端口
    svr.listen("0.0.0.0", serverport);

    // // 调试
    // // 网络上发来的Json数据
    // // n_json = {"code":"#include ....","input":"...","cpu_limit":"...","mem_limit":"..."}
    // // out_json = {"status" : "...", "reason" : "...", "stdout" : "...", "stderr" : "..."}

    // std::string in_json;
    // Json::Value in_value;
    // // R"()"  -- raw string ,不加工的数据
    // in_value["code"] = R"(#include <iostream>
    //     int main(){
    //         // aaaa
    //         // int a = 1;
    //         // a /= 0;
    //         // int *p = new int[1024*1024*50];
    //         std::cout << "你好哇"<< std::endl;
    //         return 0;
    //     }
    // )";
    // in_value["input"] = "";
    // in_value["cpu_limit"] = 1;
    // in_value["mem_limit"] = 10240*3;

    // Json::StyledWriter writer;
    // in_json = writer.write(in_value);

    // std::string out_json;
    // Compile_And_Run::Compile_Run(in_json, &out_json);

    // std::cout << out_json << std::endl;
    return 0;
}