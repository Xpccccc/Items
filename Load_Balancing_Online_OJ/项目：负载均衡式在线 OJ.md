# 项目：负载均衡式在线 OJ

## 1、整体框架

>![](https://raw.githubusercontent.com/Xpccccc/PicGo/main/data202409021157742.png)

---



## 2、所用技术和开发环境

>- `C++ STL` 标准库
>
>- `Boost` 准标准库(字符串切割)
>
>- `cpp-httplib` 第三⽅开源⽹络库
>
>- `ctemplate` 第三⽅开源前端⽹⻚渲染库
>
>- `jsoncpp` 第三⽅开源序列化、反序列化库
>
>- 负载均衡设计
>
>- 多进程、多线程
>
>- `MySQL C connect`
>
>- `Ace`前端在线编辑器(了解)
>
>- `html/css/js/jquery/ajax` (了解)

---



## 2、编写思路

>1. 先编写`compile_server`
>2. 再编写`oj_server`
>3. 基于文件版的负载均衡式在线 Oj
>4. 前端网页设计
>5. 基于MySQL版的负载均衡式在线 Oj

---



## 3、compile_server 服务设计

>提供的服务：编译并运行代码，得到格式化的相关结果。
>
>![](https://raw.githubusercontent.com/Xpccccc/PicGo/main/data202409021145604.png)

### 3.1、第一个功能: `Compile` 编译

>- `compiler.hpp`文件：
>
>```cpp
>#pragma once
>
>#include <iostream>
>#include <sys/types.h>
>#include <unistd.h>
>#include <sys/wait.h>
>#include <fcntl.h>
>
>#include "../comm/utils.hpp"
>#include "../comm/log.hpp"
>
>// 只进行编译
>namespace ns_compiler
>{
>using namespace ns_utils;
>using namespace ns_log;
>class Compiler
>{
>public:
>   Compiler() {}
>   ~Compiler() {}
>
>   // 返回值，编译成功：true，否则：false
>   static bool Compile(const std::string &filename)
>   {
>       pid_t pid = fork(); // 子进程进行程序替换来编译这个文件
>       if (pid < 0)
>       {
>           LOG(ERROR) << "内部错误，创建子进程失败" << "\n";
>           return false;
>       }
>       else if (pid == 0)
>       {
>           // 子进程：调用编译器，进行程序替换执行文件
>
>           umask(0); // 重置掩码
>
>           // 创建错误信息文件，用来保存错误信息
>           int errfd = open(PathUtil::Stderr(filename).c_str(), O_CREAT | O_WRONLY, 0644);
>           // std::cout << "debug ===== " << std::endl;
>           if (errfd < 0)
>           {
>               LOG(ERROR) << "内部错误，打开文件失败" << "\n";
>               exit(1);
>           }
>
>           // 重定向标准错误到errfd，以至于打印到显示器的信息，打印到Stderr文件
>           dup2(errfd, 2);
>
>           // g++ -o 可执行文件 源文件 -std=c++11
>           execlp("g++", "g++", "-o", PathUtil::Exe(filename).c_str(), PathUtil::Src(filename).c_str(), "-std=c++11", nullptr /*不要忘记*/);
>
>           exit(2); // 执行完就退出
>       }
>       else
>       {
>           // 父进程
>           waitpid(pid, nullptr, 0);
>           // 判断可执行文件在不在，如果在，就说明编译成功，否则失败
>           if (FileUtil::IsExistFile(PathUtil::Exe(filename).c_str()))
>           {
>               LOG(INFO) << PathUtil::Src(filename) << " 编译成功" << "\n";
>
>               return true;
>           }
>       }
>       LOG(ERROR) << "编译失败，没有形成可执行程序" << "\n";
>
>       return false;
>   }
>};
>}
>```
>
>- `Log.hpp`文件
>
>```cpp
>#pragma once
>
>#include <iostream>
>
>#include "utils.hpp"
>
>namespace ns_log
>{
>using namespace ns_utils;
>
>// 日志等级
>enum
>{
>   INFO, // 就是整数
>   DEBUG,
>   WARNING,
>   ERROR,
>   FATAL
>};
>
>// 频繁使用，使用内联函数
>inline std::ostream &Log(const std::string &level, const std::string &filename, int line)
>{
>   std::string message = "[";
>   message += level + "]";
>   message += "[";
>   message += filename + "]";
>   message += "[";
>   message += std::to_string(line) + "]";
>   message += "[";
>   message += TimeUtil::GetCurrentTime() + "]";
>   std::cout << message;
>   return std::cout;
>}
>
>// LOG(INFO) << "message"; // 开放式日志
>
>#define LOG(level) Log(#level, __FILE__, __LINE__)
>}
>```
>

---



### 3.2、第二个功能: `Run` 运行

>- `test_setrlimit`文件：测试使用限制资源函数
>
>```c++
>#include <iostream>
>#include <sys/time.h>
>#include <sys/resource.h>
>#include <unistd.h>
>#include <signal.h>
>
>void handler(int sig)
>{
>    std::cout << "sig : " << sig << std::endl;
>    exit(-1);
>}
>
>int main()
>{
>
>    for (int i = 1; i < 32; ++i)
>    {
>        signal(i, handler);
>    }
>
>    // // 限制cpu使用时间
>    // struct rlimit r_cpu;
>    // r_cpu.rlim_cur = 1; // 1秒
>    // r_cpu.rlim_max = RLIM_INFINITY;
>
>    // setrlimit(RLIMIT_CPU, &r_cpu);
>
>    // while (1)
>    //     ;
>
>    // 限制开辟空间大小
>    struct rlimit r_mem;
>    r_mem.rlim_cur = 1024 * 1024 * 40; // 40M
>    r_mem.rlim_max = RLIM_INFINITY;
>
>    setrlimit(RLIMIT_AS, &r_mem);
>
>    int count = 0;
>    while (1)
>    {
>        int *p = new int[1024 * 1024];
>        ++count;
>        std::cout << "count : " << count << std::endl;
>        sleep(1);
>    }
>    return 0;
>}
>```
>
>这里测试这两个终止信号：
>
>```bash
># cpu时间限制信号：24
>xp2@Xpccccc:~/Items/Load_Balancing_Online_Judging$ ./a.out 
>sig : 24
>
># 申请内存限制信号：6
>xp2@Xpccccc:~/Items/Load_Balancing_Online_Judging$ ./a.out 
>count : 1
>count : 2
>count : 3
>count : 4
>count : 5
>count : 6
>count : 7
>count : 8
>terminate called after throwing an instance of 'std::bad_alloc'
>  what():  std::bad_alloc
>sig : 6
>Aborted (core dumped)
>
># 信号列表
>xp2@Xpccccc:~/Items/Load_Balancing_Online_Judging$ kill -l
> 1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL       5) SIGTRAP
> 6) SIGABRT      7) SIGBUS       8) SIGFPE       9) SIGKILL     10) SIGUSR1
>11) SIGSEGV     12) SIGUSR2     13) SIGPIPE     14) SIGALRM     15) SIGTERM
>16) SIGSTKFLT   17) SIGCHLD     18) SIGCONT     19) SIGSTOP     20) SIGTSTP
>21) SIGTTIN     22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ
>26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO       30) SIGPWR
>31) SIGSYS      34) SIGRTMIN    35) SIGRTMIN+1  36) SIGRTMIN+2  37) SIGRTMIN+3
>38) SIGRTMIN+4  39) SIGRTMIN+5  40) SIGRTMIN+6  41) SIGRTMIN+7  42) SIGRTMIN+8
>43) SIGRTMIN+9  44) SIGRTMIN+10 45) SIGRTMIN+11 46) SIGRTMIN+12 47) SIGRTMIN+13
>48) SIGRTMIN+14 49) SIGRTMIN+15 50) SIGRTMAX-14 51) SIGRTMAX-13 52) SIGRTMAX-12
>53) SIGRTMAX-11 54) SIGRTMAX-10 55) SIGRTMAX-9  56) SIGRTMAX-8  57) SIGRTMAX-7
>58) SIGRTMAX-6  59) SIGRTMAX-5  60) SIGRTMAX-4  61) SIGRTMAX-3  62) SIGRTMAX-2
>63) SIGRTMAX-1  64) SIGRTMAX
>```
>
>- `runner.hpp`文件
>
>```cpp
>#pragma once
>
>#include <iostream>
>#include <sys/types.h>
>#include <unistd.h>
>#include <sys/stat.h>
>#include <sys/wait.h>
>#include <fcntl.h>
>#include <sys/time.h>
>#include <sys/resource.h>
>
>#include "../comm/log.hpp"
>#include "../comm/utils.hpp"
>
>namespace ns_runner
>{
>    using namespace ns_log;
>    using namespace ns_utils;
>    class Runner
>    {
>    public:
>        static void SetRlimit(int cpu_limit, int mem_limit)
>        {
>            // 限制cpu使用时间
>            struct rlimit r_cpu;
>            r_cpu.rlim_cur = cpu_limit; // 1秒
>            r_cpu.rlim_max = RLIM_INFINITY;
>
>            setrlimit(RLIMIT_CPU, &r_cpu);
>
>            // 限制开辟空间大小
>            struct rlimit r_mem;
>            r_mem.rlim_cur = mem_limit * 1024; // KB
>            r_mem.rlim_max = RLIM_INFINITY;
>
>            setrlimit(RLIMIT_AS, &r_mem);
>        }
>
>    public:
>        // 信号 ，0表示运行成功；-1表示内部错误，其他就是对应的信号值
>
>        /*
>            filename: 文件名，不带后缀
>            cpu_limit: cpu限制使用时间（秒）
>            mem_limit: 内存开辟限制（KB）
>        */
>        static int runner(std::string &filename, int cpu_limit, int mem_limit)
>        {
>            /*
>                因为运行可执行文件，需要保存运行结果
>                运行结果：1.执行失败 2.执行成功
>                1.执行失败 把错误信息放在.stderr文件中
>                2.执行成功 把执行结果放在.stdout文件中
>                这里加一个可扩展的模块，比如，对于用户，可能会自己添加测试用例，那么他输入的用例需要放到.stdin文件中
>            */
>
>            // 这里执行对应的文件，我们不关心他运行后的结果，只关心它运行成功或失败
>
>            umask(0);
>            int _stdin_fd = open(PathUtil::Stdin(filename).c_str(), O_CREAT | O_WRONLY, 0644);
>            int _stdout_fd = open(PathUtil::Stdout(filename).c_str(), O_CREAT | O_WRONLY, 0644);
>            int _stderr_fd = open(PathUtil::Stderr(filename).c_str(), O_CREAT | O_WRONLY, 0644);
>
>            // 必须得先保证能打开这些文件，如果打不开，后续动作将没有意义（读取不了文件，对比不了结果）
>            if (_stdin_fd < 0 || _stdout_fd < 0 || _stderr_fd < 0)
>            {
>                LOG(FATAL) << "打开文件失败" << "\n";
>                return -1; // 代表打开文件失败
>            }
>            pid_t pid = fork();
>            if (pid < 0)
>            {
>                ::close(_stdin_fd);
>                ::close(_stdout_fd);
>                ::close(_stderr_fd);
>                LOG(FATAL) << " 内部错误，创建子进程错误" << "\n";
>                return -2; // 代码创建子进程失败
>            }
>            else if (pid == 0)
>            {
>                // 重定向
>                dup2(_stdin_fd, 0);
>                dup2(_stdout_fd, 1);
>                dup2(_stderr_fd, 2);
>
>                // 设置限制
>                SetRlimit(cpu_limit, mem_limit); // 这里出问题也是使用信号
>
>                execl(PathUtil::Exe(filename).c_str() /*文件路径*/, PathUtil::Exe(filename).c_str() /*执行文件*/, nullptr);
>
>                exit(1);
>            }
>            else
>            {
>                ::close(_stdin_fd);
>                ::close(_stdout_fd);
>                ::close(_stderr_fd);
>                int status = 0;
>                waitpid(pid, &status, 0);
>
>                return status & 0x7f; // 信号 ，0表示运行成功；-1表示内部错误，其他就是对应的信号值
>            }
>            return -3; // 执行错误
>        }
>    };
>}
>```

---



### 3.3、第三个功能: `Compile_Run` 编译并运行

>![](https://raw.githubusercontent.com/Xpccccc/PicGo/main/data202409041654169.png)
>
>- `compile_run.hpp`文件：
>
>```cpp
>#pragma once
>
>#include <jsoncpp/json/json.h>
>
>#include "compiler.hpp"
>#include "runner.hpp"
>
>#include "../comm/utils.hpp"
>
>namespace ns_complie_run
>{
>
>    using namespace ns_compiler;
>    using namespace ns_runner;
>    using namespace ns_utils;
>    class Compile_And_Run
>    {
>
>    public:
>        static std::string StatusToDesc(int status, const std::string &filename)
>        {
>            std::string desc;
>            switch (status)
>            {
>            case 0:
>                desc = "编译运行成功！";
>                break;
>            case -1:
>                desc = "代码文件为空";
>                break;
>            case -2:
>                desc = "未知错误";
>                break;
>            case -3:
>                FileUtil::ReadFile(PathUtil::Compile_err(filename), &desc, true); // 读取编译错误的文件内容
>                break;
>            case SIGABRT:
>                desc = "申请空间超过限制"; // 6
>                break;
>            case SIGXCPU:
>                desc = "CPU使用时间超过限制"; // 24
>                break;
>            case SIGFPE:
>                desc = "浮点数溢出"; // 8
>                break;
>            case SIGSEGV:
>                desc = "段错误"; // 11
>                break;
>            default:
>                desc = "未知: " + std::to_string(status);
>                break;
>            }
>            return desc;
>        }
>
>        static void RemoveTmpFile(const std::string &filename)
>        {
>            std::string _src = PathUtil::Src(filename);
>            if (FileUtil::IsExistFile(_src))
>            {
>                unlink(_src.c_str());
>            }
>            std::string _stdin = PathUtil::Stdin(filename);
>            if (FileUtil::IsExistFile(_stdin))
>            {
>                unlink(_stdin.c_str());
>            }
>            std::string _stdout = PathUtil::Stdout(filename);
>            if (FileUtil::IsExistFile(_stdout))
>            {
>                unlink(_stdout.c_str());
>            }
>            std::string _stderr = PathUtil::Stderr(filename);
>            if (FileUtil::IsExistFile(_stderr))
>            {
>                unlink(_stderr.c_str());
>            }
>            std::string _compile_err = PathUtil::Compile_err(filename);
>            if (FileUtil::IsExistFile(_compile_err))
>            {
>                unlink(_compile_err.c_str());
>            }
>            std::string _exe = PathUtil::Exe(filename);
>            if (FileUtil::IsExistFile(_exe))
>            {
>                unlink(_exe.c_str());
>            }
>        }
>
>    public:
>        /*
>            输入：
>            code: 用户提交的代码
>            input: 用户自己提交的用例
>
>            in_json = {"code":"#include ....","input":"...","cpu_limit":"...","mem_limit":"..."}
>
>            输出：
>            status: 状态码
>            reason: 错误原因
>
>            下面两个选择填入
>            stdout: 程序运行完的结果
>            stderr: 程序异常的原因
>            out_json = {"status":"...","reason":"...","stdout":"...","stderr":"..."}
>        */
>
>        // 这里的in_json是网络传输过来
>        static void Compile_Run(std::string &in_json, std::string *out_json)
>        {
>            Json::Value in_value;
>            Json::Reader reader;
>
>            reader.parse(in_json, in_value);
>
>            std::string code = in_value["code"].asCString();
>            std::string input = in_value["input"].asCString();
>            int cpu_limit = in_value["cpu_limit"].asInt();
>            int mem_limit = in_value["mem_limit"].asInt();
>
>            int status = 0;
>            Json::Value out_value;
>            std::string filename;
>            int run_code;
>
>            if (code.size() == 0)
>            {
>                // TODO
>                status = -1; // 代码文件为空
>                goto END;
>            }
>
>            // 形成的文件得具有唯一性
>            filename = FileUtil::UniqueFileName(code);
>
>            // 形成临时src文件
>            // 把code里面的内容放到Src文件中
>            if (!FileUtil::WriteFile(PathUtil::Src(filename), code))
>            {
>                status = -2; // 写文件失败
>                goto END;
>            }
>
>            // 编译
>            if (!Compiler::Compile(filename))
>            {
>                status = -3; // 编译失败
>                goto END;
>            }
>
>            // 执行
>            run_code = Runner::Run(filename, cpu_limit, mem_limit);
>            status = run_code;
>            if (run_code < 0)
>            {
>                status = -2; // 未知错误
>            }
>            else // run_code >= 0
>            {
>                // 运行成功就是0
>                status = run_code;
>            }
>
>        END:
>            out_value["status"] = status;
>            out_value["reason"] = StatusToDesc(status, filename);
>            if (status == 0)
>            {
>                // 整个过程全部成功
>                // 运行结果在stdout文件
>                std::string _stdout;
>                FileUtil::ReadFile(PathUtil::Stdout(filename), &_stdout, true); // 需要\n
>                out_value["stdout"] = _stdout;
>
>                // 运行错误在stderr文件 -- 存在一点问题，运行错误不会进来这个文件
>                std::string _stderr;
>                FileUtil::ReadFile(PathUtil::Stderr(filename), &_stderr, true); // 需要\n
>                out_value["stderr"] = _stderr;
>            }
>
>            // 序列化
>            Json::StyledWriter witer;
>            *out_json = witer.write(out_value);
>
>            // 已经得到结果了，直接移除临时文件
>            RemoveTmpFile(filename);
>        }
>    };
>}
>```
>
>测试功能：
>
>- `complile_server.cc`文件
>
>```cpp
>#include <jsoncpp/json/json.h>
>
>#include "compile_run.hpp"
>
>using namespace ns_complie_run;
>
>int main()
>{
>    // 调试
>    // 网络上发来的Json数据
>    // n_json = {"code":"#include ....","input":"...","cpu_limit":"...","mem_limit":"..."}
>    // out_json = {"status" : "...", "reason" : "...", "stdout" : "...", "stderr" : "..."}
>
>    std::string in_json;
>    Json::Value in_value;
>    // R"()"  -- raw string ,不加工的数据
>    in_value["code"] = R"(#include <iostream>
>        int main(){
>            // aaaa
>            // int a = 1;
>            // a /= 0;
>            // int *p = new int[1024*1024*50];
>            std::cout << "你好哇"<< std::endl;
>            return 0;
>        }
>    )";
>    in_value["input"] = "";
>    in_value["cpu_limit"] = 1;
>    in_value["mem_limit"] = 10240*3;
>
>    Json::StyledWriter writer;
>    in_json = writer.write(in_value);
>
>    std::string out_json;
>    Compile_And_Run::Compile_Run(in_json, &out_json);
>
>    std::cout << out_json << std::endl;
>    return 0;
>}
>```

----



### 3.4、第四个功能：把 `compile_run` 打包成网络服务

>引入 `cpp-httplib` 工具
>
>直接添加 `httplib.h` 库，可以直接使用其功能。
>
>```cpp
>#include <jsoncpp/json/json.h>
>
>#include "../comm/httplib.h"
>#include "compile_run.hpp"
>
>using namespace ns_complie_run;
>using namespace httplib;
>
>int main(int argc, char *argv[])
>{
>    if (argc != 2)
>    {
>        std::cout << "Usage : \n        ./compile_server port" << std::endl;
>        return 1;
>    }
>
>    uint16_t serverport = std::stoi(argv[1]);
>
>    std::cout << serverport << std::endl;
>    // 把compile_run打包成网络服务
>    Server svr;
>    svr.Get("/hello", [](const Request &req, Response &resp)
>            { resp.set_content("hello httplib,你好啊", "text/plain;charset=utf-8"); });
>
>    svr.Post("/compile_run", [](const Request &req, Response &resp)
>             {
>        std::string in_json = req.body;
>        std::string out_json;
>
>        if (!in_json.empty())
>        {
>            Compile_And_Run::Compile_Run(in_json, &out_json);
>            resp.set_content(out_json,"application/json;charset=utf-8");
>        } });
>
>    // 绑定特定端口，多次启动可以绑定多个端口  -- 这里端口记得在服务器开启，不然可能访问不了
>    svr.listen("0.0.0.0", serverport);
>    return 0;
>}
>```
>
>使用 `postman` 软件可以测试
>
>![](https://raw.githubusercontent.com/Xpccccc/PicGo/main/data202409051128719.png)

----



## 4、基于MVC结构的 oj_server 服务设计

>MVC结构：
>
>- M ：Model ，通常是和数据交互的模块，比如对题库进行增删查改（文件版/MySQL）。
>- V ：View ，通常是拿到数据之后，要进行构建网页，渲染网页内容，展示给用户的（浏览器）。
>- C ：Control ，控制器，本质上就是我们的核心业务逻辑。
>
>`oj_server` 服务设计：本质就是建立一个小型网站。

---



### 4.1、第一个功能: 用户请求的服务器路由功能

>- `oj_server.hpp`文件：
>
>```cpp
>#include <iostream>
>
>#include "../comm/httplib.h"
>
>using namespace httplib;
>
>int main()
>{
>    // 用户请求的服务路由功能
>    Server svr;
>
>    // 获取题目列表
>    svr.Get("/questions", [](const Request &req, Response &resp)
>            { resp.set_content("获取题目列表", "text/plain;charset=utf-8"); });
>
>    // 获取指定题目
>    svr.Get(R"(/question/(\d+))", [](const Request &req, Response &resp)
>            { 
>                std::string number = req.matches[1];
>                resp.set_content("获取指定题目 : " + number, "text/plain;charset=utf-8"); });
>    
>    // 用户提交代码，使用我们的判题功能 (1.测试用例 2.compile_run)
>    svr.Get(R"(/judge/(\d+))", [](const Request &req, Response &resp) {
>        
>    });
>
>    svr.listen("0.0.0.0", 8080);
>
>    return 0;
>}
>```

---



### 4.2、第二个功能: Model 功能，提供对数据进行操作

>- `oj_model.hpp`文件：
>
>```cpp
>#pragma once
>
>#include <string>
>#include <vector>
>#include <fstream>
>#include <unordered_map>
>
>#include <cassert>
>#include <cstdlib>
>
>#include "../comm/utils.hpp"
>#include "../comm/log.hpp"
>
>namespace ns_model
>{
>    using namespace ns_log;
>    using namespace ns_utils;
>    struct Question
>    {
>        std::string number; // 题目编号
>        std::string title;  // 题目标题
>        std::string level;  // 题目等级
>        int cpu_limit;      // 时间要求
>        int mem_limit;      // 空间要求
>
>        std::string desc;   // 题目描述
>        std::string header; // 题目预设给的代码
>        std::string tail;   // 测试用例代码，需要和header拼接再编译运行
>    };
>
>    const std::string questions_path = "./questions/questions.list"; // 题目列表配置文件
>    const std::string question_path = "./questions/";                // 单个题目的路径
>
>    class Model
>    {
>    private:
>        // 题号：题目细节
>        std::unordered_map<std::string, Question> _questions;
>
>    public:
>        Model()
>        {
>            assert(LoadAllQuestion(questions_path));
>        }
>
>        // 加载配置文件
>        bool LoadAllQuestion(const std::string &question_list)
>        {
>            std::ifstream in(question_list);
>            if (!in.is_open())
>            {
>                LOG(FATAL) << "加载题库失败，请检查是否有该题库" << "\n";
>                return false;
>            }
>            std::string line;
>            while (getline(in, line))
>            {
>                std::vector<std::string> tokens;
>                StringUtil::StringSplit(line, &tokens, " ");
>                if (tokens.size() != 5)
>                {
>                    // 1 回文数 简单 1 30000
>                    // 没有读到五个数，有可能是写少了或者多了
>                    LOG(WARNING) << "加载单个题目失败，请检查格式" << "\n";
>                    continue;
>                }
>                Question q;
>                q.number = tokens[0];
>                q.title = tokens[1];
>                q.level = tokens[2];
>                q.cpu_limit = std::atoi(tokens[3].c_str());
>                q.mem_limit = std::atoi(tokens[4].c_str());
>
>                FileUtil::ReadFile(question_list + q.number + "desc.txt", &q.desc, true);
>                FileUtil::ReadFile(question_list + q.number + "header.hpp", &q.header, true);
>                FileUtil::ReadFile(question_list + q.number + "tail.hpp", &q.tail, true);
>
>                _questions.insert({q.number, q});
>            }
>            LOG(INFO) << "加载题库成功！" << "\n";
>            in.close();
>        }
>
>        // 获取所有题目列表
>        bool GetAllQuestions(std::vector<Question> *out)
>        {
>            if (_questions.size() == 0)
>            {
>                LOG(ERROR) << "用户获取题库失败" << "\n";
>                return false;
>            }
>            for (const auto &q : _questions)
>            {
>                out->push_back(q.second);
>            }
>            return true;
>        }
>
>        // 获取具体的一道题
>        bool GetOneQuestion(const std::string &number, Question *out)
>        {
>            auto iter = _questions.find(number);
>            if (iter == _questions.end())
>            {
>                LOG(ERROR) << "用户获取题目失败，题目编号：" << number << "\n";
>                return false;
>            }
>
>            *out = iter->second;
>            return true;
>        }
>
>        ~Model() {}
>    };
>}
>```

----



### 4.3、第三个功能: Control 功能，逻辑控制模块（<font color=red>含负载均衡模块代码</font>）

>- `oj_control.hpp`文件：
>
>```cpp
>#pragma once
>
>#include <iostream>
>#include <jsoncpp/json/json.h>
>#include <vector>
>#include <mutex>
>#include <unistd.h>
>#include <fstream>
>#include <cassert>
>#include <algorithm>
>
>// #include "oj_model.hpp"
>#include "oj_model_MySQL.hpp"
>#include "oj_view.hpp"
>#include "../comm/log.hpp"
>#include "../comm/utils.hpp"
>#include "../comm/httplib.h"
>
>namespace ns_control
>{
>    using namespace ns_model;
>    using namespace ns_view;
>    using namespace ns_log;
>    using namespace ns_utils;
>    using namespace httplib;
>
>    // 提供编译运行服务的主机
>    class Machine
>    {
>    public:
>        std::string _ip;
>        uint16_t _port;
>        uint64_t _load;   // 负载量
>        std::mutex *_mtx; // 对_load进行加锁
>
>    public:
>        Machine() : _ip(""), _port(0), _load(0), _mtx(nullptr) {}
>
>        // 增加负载
>        void IncLoad()
>        {
>            if (_mtx)
>                _mtx->lock();
>            ++_load;
>            if (_mtx)
>                _mtx->unlock();
>        }
>
>        // 降低负载
>        void DecLoad()
>        {
>            if (_mtx)
>                _mtx->lock();
>            --_load;
>            if (_mtx)
>                _mtx->unlock();
>        }
>
>        // 读取load，没有太大意义，只是为了统一接口
>        uint64_t Load()
>        {
>            uint64_t load = 0;
>            if (_mtx)
>                _mtx->lock();
>            load = _load;
>            if (_mtx)
>                _mtx->unlock();
>            return load;
>        }
>
>        // 负载清零，一台主机下线了，它的负载得清零
>        void ResetLoad()
>        {
>            if (_mtx)
>                _mtx->lock();
>            _load = 0;
>            if (_mtx)
>                _mtx->unlock();
>        }
>        ~Machine() {}
>    };
>
>    const std::string machine_conf = "./conf/machine_list.conf";
>
>    // 提供负载均衡选择主机
>    class LoadBalance
>    {
>    private:
>        std::vector<Machine> _machines; // 提供负载均衡的主机群 -- 以下表在标识每一台主机
>        std::vector<int> _online;       // 在线的主机
>        std::vector<int> _offline;      // 离线的主机
>        std::mutex _mtx;                // 对负载均衡选择加锁，因为可能同时到来多个用户提交代码，而_machines等是临界资源（共享的）
>    public:
>        LoadBalance()
>        {
>            assert(LoadMachine(machine_conf));
>            LOG(INFO) << "加载配置文件 " << machine_conf << " 成功" << "\n";
>        }
>
>        bool LoadMachine(const std::string &machine_file)
>        {
>            std::ifstream in(machine_file);
>            if (!in.is_open())
>            {
>                LOG(FATAL) << "加载配置文件 " << machine_file << " 失败" << "\n";
>                return false;
>            }
>
>            std::string line;
>            while (getline(in, line))
>            {
>                std::vector<std::string> tokens;
>                StringUtil::StringSplit(line, &tokens, ":");
>                if (tokens.size() != 2)
>                {
>                    LOG(WARNING) << "加载配置行 " << line << " 失败" << "\n";
>                    continue;
>                }
>                Machine m;
>                m._ip = tokens[0];
>                m._port = std::atoi(tokens[1].c_str());
>                m._load = 0;
>                m._mtx = new std::mutex();
>                _online.push_back(_machines.size()); // 先把上线的主机的下标对应主机 0 <-> 第一台 ...
>                _machines.push_back(m);
>            }
>            return true;
>        }
>
>        // 智能选择负载最小的主机
>        bool SmartChioce(int *id, Machine **m)
>        {
>            _mtx.lock();
>
>            // 负载均衡的算法
>            // 1.随机+hash
>            // 2.轮询+hash
>            int num_online = _online.size(); // 在线的主机
>            if (num_online == 0)
>            {
>                _mtx.unlock(); // 出去前先解锁
>                LOG(FATAL) << "没有在线的主机，赶紧检查！" << "\n";
>                return false;
>            }
>
>            uint64_t min_load = _machines[_online[0]].Load();
>            *id = _online[0];
>            *m = &_machines[_online[0]];
>            for (int i = 1; i < num_online; ++i)
>            {
>                uint64_t cur_load = _machines[_online[i]].Load();
>                if (cur_load < min_load)
>                {
>                    min_load = cur_load;
>                    *id = _online[i];
>                    *m = &_machines[_online[i]]; // 要这台主机的地址
>                }
>            }
>
>            _mtx.unlock();
>            return true;
>        }
>
>        void OnlineMachine()
>        {
>            _mtx.lock();
>            // 把离线主机添加到在线主机
>            _online.insert(_online.end(), _offline.begin(), _offline.end());
>            _offline.erase(_offline.begin(), _offline.end());
>            _mtx.unlock();
>            LOG(INFO) << "所有主机上线啦！" << "\n";
>        }
>
>        // 让当前主机进入下线状态
>        void OfflineMachine(int id)
>        {
>            _mtx.lock();
>            for (auto iter = _online.begin(); iter != _online.end(); ++iter)
>            {
>                if (*iter == id)
>                {
>                    _machines[id].ResetLoad(); // 清空当前主机的负载
>                    _online.erase(iter);
>                    _offline.push_back(id);
>                    break; // id唯一
>                }
>            }
>            _mtx.unlock();
>        }
>
>        void ShowMachines()
>        {
>            std::cout << "当前在线的主机 : ";
>            for (auto id : _online)
>            {
>                std::cout << id << " ";
>            }
>            std::cout << std::endl;
>
>            std::cout << "当前离线线的主机 : ";
>            for (auto id : _offline)
>            {
>                std::cout << id << " ";
>            }
>            std::cout << std::endl;
>        }
>
>        ~LoadBalance() {}
>    };
>
>    class Control
>    {
>    private:
>        Model _model;              // 提供后台数据
>        View _view;                // 提供html渲染
>        LoadBalance _load_balance; // 提供核心负载均衡器
>
>    public:
>        Control() {}
>
>        void RecoveryMachines()
>        {
>            _load_balance.OnlineMachine();
>        }
>
>        bool AllQuestions(std::string *html)
>        {
>            bool ret = true;
>            // 获取所有题目
>            std::vector<Question> all;
>            if (_model.GetAllQuestions(&all))
>            {
>                // 渲染网页
>                // 排序
>                sort(all.begin(), all.end(), [](const Question &q1, const Question &q2)
>                     { return std::atoi(q1.number.c_str()) < std::atoi(q2.number.c_str()); });
>                _view.DrawAllQuestions(all, html);
>            }
>            else
>            {
>                *html = "加载题库失败";
>                ret = false;
>            }
>            return ret;
>        }
>        bool OneQuestion(const std::string &number, std::string *html)
>        {
>            bool ret = true;
>            Question q;
>            if (_model.GetOneQuestion(number, &q))
>            {
>                // 渲染网页
>                _view.DrawOneQuestion(q, html);
>            }
>            else
>            {
>                *html = "加载题目 " + number + " 失败";
>                ret = false;
>            }
>            return ret;
>        }
>
>        void Judge(const std::string &number, const std::string &in_json, std::string *out_json)
>        {
>            // 0.根据题目编号，拿到题目细节
>            Question q;
>            _model.GetOneQuestion(number, &q);
>
>            // 1.对in_json进行反序列化，得到code，input
>            Json::Value in_value;
>            Json::Reader reader;
>            reader.parse(in_json, in_value);
>            std::string code = in_value["code"].asCString();
>            std::string input = in_value["input"].asCString();
>
>            // 2.拼接代码，header+tail
>            Json::Value compile_run_value;
>            compile_run_value["code"] = code + "\n" + q.tail; // 记得加\n，不然可能存在编译的时候和ifndef COMPILE 共一行
>            compile_run_value["input"] = input;
>            compile_run_value["cpu_limit"] = q.cpu_limit;
>            compile_run_value["mem_limit"] = q.mem_limit;
>            Json::StyledWriter writer;
>            std::string compile_run_string = writer.write(compile_run_value);
>
>            // 3.选择负载最低的主机
>            // 规则：一直选择，直到主机可用，否则就是主机已经全部挂掉
>            while (true)
>            {
>                int id = 0;
>                Machine *m = nullptr;
>                if (!_load_balance.SmartChioce(&id, &m))
>                {
>                    break; // 主机全部挂掉了
>                }
>
>                // 4.发起http请求，得到结果
>                Client cli(m->_ip, m->_port);
>                m->IncLoad(); // 用的时候增加负载值
>                LOG(INFO) << "连接主机成功 , 主机id : " << id << " 详情 : " << m->_ip << ":" << m->_port << " 负载 : " << m->Load() << "\n";
>
>                // inline Result Client::Post(const char *path, const std::string &body, const char *content_type)
>                auto res = cli.Post("/compile_run", compile_run_string, "application/json;charset=utf-8");
>                if (res) // 编译运行
>                {
>                    if (res->status == 200)
>                    {
>                        // 请求成功
>                        // 5.将结果给out_json
>                        *out_json = res->body;
>                        m->DecLoad(); // 用完后减少负载值
>                        break;
>                    }
>                    m->DecLoad();
>                }
>                else
>                {
>                    // 请求失败
>                    LOG(WARNING) << "当前请求的主机id : " << id << " 详情 : " << m->_ip << ":" << m->_port << " 可能已经离线" << "\n";
>                    _load_balance.OfflineMachine(id); // 当前主机下线
>                    _load_balance.ShowMachines();     // 用来测试
>                }
>                sleep(1);
>            }
>        }
>
>        ~Control() {}
>    };
>}
>```
>

---



### 4.4、第四个功能：View 功能，页面渲染模块

>- `oj_view.hpp`文件了：
>
>```cpp
>#pragma once
>
>#include <ctemplate/template.h>
>
>#include <string>
>#include <vector>
>
>#include "oj_model.hpp"
>
>namespace ns_view
>{
>    using namespace ns_model;
>
>    const std::string draw_path = "./template_html/";
>    class View
>    {
>    public:
>        void DrawAllQuestions(std::vector<Question> &all, std::string *html)
>        {
>            std::string in_html = draw_path + "all_questions.html";
>            // 形成数据字典
>            ctemplate::TemplateDictionary root("all");
>
>            for (const auto &q : all)
>            {
>                // 形成子数据字典
>                ctemplate::TemplateDictionary *sub = root.AddSectionDictionary("question_list");
>                sub->SetValue("number", q.number);
>                sub->SetValue("title", q.title);
>                sub->SetValue("level", q.level);
>            }
>
>            // 获取被渲染的网页对象
>            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(in_html, ctemplate::DO_NOT_STRIP);
>
>            // 添加字典数据到网页中
>            tpl->Expand(html, &root);
>        }
>
>        void DrawOneQuestion(const Question &q, std::string *html)
>        {
>            std::string in_html = draw_path + "question.html";
>
>            ctemplate::TemplateDictionary root("one");
>
>            root.SetValue("number", q.number);
>            root.SetValue("title", q.title);
>            root.SetValue("level", q.level);
>
>            root.SetValue("desc", q.desc);
>            root.SetValue("pre_code", q.header);
>
>            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(in_html, ctemplate::DO_NOT_STRIP);
>
>            tpl->Expand(html, &root);
>        }
>    };
>}
>```
>

----



## 5、基于 MySQL 版的 Model 功能

>`oj_model_MySQL.hpp`文件：
>
>```mysql
>#pragma once
>
>#include <string>
>#include <vector>
>#include <fstream>
>#include <unordered_map>
>
>#include <cassert>
>#include <cstdlib>
>
>#include <mysql/mysql.h>
>
>#include "../comm/utils.hpp"
>#include "../comm/log.hpp"
>
>namespace ns_model
>{
>    using namespace ns_log;
>    using namespace ns_utils;
>    struct Question
>    {
>        std::string number; // 题目编号 -- 唯一
>        std::string title;  // 题目标题
>        std::string level;  // 题目难度
>        int cpu_limit;      // 时间要求
>        int mem_limit;      // 空间要求
>
>        std::string desc;   // 题目描述
>        std::string header; // 题目预设给的代码
>        std::string tail;   // 测试用例代码，需要和header拼接再编译运行
>    };
>
>    const std::string questions_table = "oj_questions"; // 数据库表名
>    const std::string host = "127.0.0.1";
>    const uint16_t port = 3306;
>    const std::string passwd = "xp1014647664";
>    const std::string db = "oj";
>    const std::string user = "oj_client";
>
>    class Model
>    {
>    private:
>        bool MySQLQuery(const std::string &sql, std::vector<Question> *out)
>        {
>            // 创建句柄
>            MYSQL *my = mysql_init(nullptr);
>
>            // 连接数据库
>            if (nullptr == mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0))
>            {
>                LOG(FATAL) << "连接数据库失败!" << "\n";
>                mysql_close(my); // 释放句柄
>                return false;
>            }
>
>            LOG(INFO) << "连接数据库成功!" << "\n";
>
>            mysql_set_character_set(my, "utf8");
>
>            // 执行语句
>            if (0 != mysql_query(my, sql.c_str()))
>            {
>                LOG(WARNING) << "执行语句 " << sql << " 失败!" << "\n";
>                mysql_close(my); // 释放句柄
>                return false;
>            }
>
>            // 提取结果
>            MYSQL_RES *res = mysql_store_result(my);
>            if (res == nullptr)
>            {
>                LOG(WARNING) << "查询未返回结果集" << "\n";
>                mysql_close(my);
>                return false;
>            }
>
>            // 获取行列数
>            int rows = mysql_num_rows(res);
>
>            // int cols = mysql_num_fields(res);
>
>            for (int i = 0; i < rows; ++i)
>            {
>                // 获取每一行
>                Question q;
>                MYSQL_ROW row = mysql_fetch_row(res);
>                q.number = row[0];
>                q.title = row[1];
>                q.level = row[2];
>                q.cpu_limit = atoi(row[3]);
>                q.mem_limit = atoi(row[4]);
>                q.desc = row[5];
>                q.header = row[6];
>                q.tail = row[7];
>                out->push_back(q);
>            }
>
>            mysql_free_result(res);
>            mysql_close(my);
>            return true;
>        }
>
>    public:
>        Model() {}
>
>        // 获取所有题目列表
>        bool GetAllQuestions(std::vector<Question> *out)
>        {
>            std::string sql = "select * from ";
>            sql += questions_table;
>            return MySQLQuery(sql, out);
>        }
>
>        // 获取具体的一道题
>        bool GetOneQuestion(const std::string &number, Question *out)
>        {
>            bool ret = false;
>            std::string sql = "select * from ";
>            sql += questions_table;
>            sql += " where number=";
>            sql += number;
>
>            std::vector<Question> result;
>            if (MySQLQuery(sql, &result))
>            {
>                if (result.size() == 1)
>                {
>                    *out = result[0];
>                    return true;
>                }
>            }
>
>            return ret;
>        }
>
>        ~Model() {}
>    };
>}
>```

----



## 6、前端网页设计 html

>- 主页：`index.html`
>
>```html
><!DOCTYPE html>
><html lang="en">
>
><head>
>    <meta charset="UTF-8">
>    <meta http-equiv="X-UA-Compatible" content="IE=edge">
>    <meta name="viewport" content="width=device-width, initial-scale=1.0">
>    <title>这是我的个人OJ系统</title>
>    <style>
>        /* 消除默认样式 */
>        * {
>            margin: 0;
>            padding: 0;
>            box-sizing: border-box;
>        }
>
>        html,
>        body {
>            width: 100%;
>            height: 100%;
>            font-family: 'Arial', sans-serif;
>            background: linear-gradient(to bottom, #f0f4f8, #d9e2ec);
>        }
>
>        .container {
>            min-height: 100vh;
>            display: flex;
>            flex-direction: column;
>        }
>
>        /* 导航栏样式 */
>        .navbar {
>            display: flex;
>            justify-content: space-between;
>            align-items: center;
>            background-color: #333;
>            padding: 0 20px;
>            height: 60px;
>        }
>
>        /* 左侧导航菜单 */
>        .navbar-left {
>            display: flex;
>        }
>
>        .navbar-left a {
>            color: white;
>            font-size: 18px;
>            text-decoration: none;
>            padding: 14px 20px;
>            transition: background-color 0.3s ease;
>        }
>
>        .navbar-left a:hover {
>            background-color: #4CAF50;
>            border-radius: 4px;
>        }
>
>        /* 右侧登录按钮 */
>        .navbar-right .login {
>            color: white;
>            font-size: 18px;
>            text-decoration: none;
>            padding: 14px 20px;
>            background-color: #4CAF50;
>            border-radius: 4px;
>            transition: background-color 0.3s ease;
>        }
>
>        .navbar-right .login:hover {
>            background-color: #45a049;
>        }
>
>        /* 内容样式 */
>        .content {
>            flex: 1;
>            display: flex;
>            flex-direction: column;
>            justify-content: center;
>            align-items: center;
>            text-align: center;
>            padding: 20px;
>        }
>
>        .content h1 {
>            font-size: 36px;
>            color: #333;
>            margin-bottom: 20px;
>            letter-spacing: 2px;
>        }
>
>        .content p {
>            font-size: 20px;
>            color: #666;
>            margin-bottom: 30px;
>        }
>
>        .content a {
>            font-size: 22px;
>            padding: 10px 25px;
>            color: white;
>            background-color: #4CAF50;
>            border-radius: 5px;
>            text-decoration: none;
>            transition: background-color 0.3s ease;
>        }
>
>        .content a:hover {
>            background-color: #45a049;
>        }
>    </style>
></head>
>
><body>
>    <div class="container">
>        <!-- 导航栏 -->
>        <div class="navbar">
>            <!-- 左侧导航链接 -->
>            <div class="navbar-left">
>                <a href="/">首页</a>
>                <a href="/all_questions">题库</a>
>                <a href="#">竞赛</a>
>                <a href="#">讨论</a>
>                <a href="#">求职</a>
>            </div>
>            <!-- 右侧登录按钮 -->
>            <div class="navbar-right">
>                <a class="login" href="#">登录</a>
>            </div>
>        </div>
>
>        <!-- 内容区 -->
>        <div class="content">
>            <h1>欢迎来到我的Online Judge平台</h1>
>            <p>这是一个我个人独立开发的在线OJ平台，享受编程的乐趣吧！</p>
>            <a href="/all_questions">点击我开始编程啦!</a>
>        </div>
>    </div>
></body>
>
></html>
>
>```
>
>- 题目列表页：`all_questions.html`
>
>```html
><!DOCTYPE html>
><html lang="en">
>
><head>
>    <meta charset="UTF-8">
>    <meta http-equiv="X-UA-Compatible" content="IE=edge">
>    <meta name="viewport" content="width=device-width, initial-scale=1.0">
>    <title>在线OJ-题目列表</title>
>    <style>
>        /* 清除默认样式 */
>        * {
>            margin: 0;
>            padding: 0;
>            box-sizing: border-box;
>        }
>
>        body,
>        html {
>            font-family: 'Arial', sans-serif;
>            height: 100%;
>            background: linear-gradient(to right, #eef2f3, #8e9eab);
>        }
>
>        .container {
>            display: flex;
>            flex-direction: column;
>            min-height: 100vh;
>        }
>
>        /* 导航栏样式 */
>        .navbar {
>            display: flex;
>            justify-content: space-between;
>            align-items: center;
>            background-color: #333;
>            padding: 0 20px;
>            height: 60px;
>        }
>
>        /* 左侧导航菜单 */
>        .navbar-left {
>            display: flex;
>        }
>
>        .navbar-left a {
>            color: white;
>            font-size: 18px;
>            text-decoration: none;
>            padding: 14px 20px;
>            transition: background-color 0.3s ease;
>        }
>
>        .navbar-left a:hover {
>            background-color: #4CAF50;
>            border-radius: 4px;
>        }
>
>        /* 右侧登录按钮 */
>        .navbar-right .login {
>            color: white;
>            font-size: 18px;
>            text-decoration: none;
>            padding: 14px 20px;
>            background-color: #4CAF50;
>            border-radius: 4px;
>            transition: background-color 0.3s ease;
>        }
>
>        .navbar-right .login:hover {
>            background-color: #45a049;
>        }
>
>        /* 题目列表 */
>        .question_list {
>            flex: 1;
>            padding: 50px 20px;
>            display: flex;
>            flex-direction: column;
>            align-items: center;
>        }
>
>        .question_list h1 {
>            font-size: 36px;
>            color: #333;
>            margin-bottom: 30px;
>        }
>
>        /* 表格样式 */
>        table {
>            width: 80%;
>            border-collapse: collapse;
>            background-color: white;
>            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
>            border-radius: 10px;
>            overflow: hidden;
>        }
>
>        th,
>        td {
>            padding: 15px;
>            text-align: center;
>            border-bottom: 1px solid #ddd;
>        }
>
>        th {
>            background-color: #4CAF50;
>            color: white;
>            font-size: 18px;
>        }
>
>        tr:hover {
>            background-color: #f2f2f2;
>        }
>
>        .item a {
>            color: #333;
>            text-decoration: none;
>            transition: color 0.3s ease, text-decoration 0.3s ease;
>        }
>
>        .item a:hover {
>            color: #4CAF50;
>            text-decoration: underline;
>        }
>
>        /* 页脚 */
>        .footer {
>            height: 50px;
>            background-color: #333;
>            color: white;
>            display: flex;
>            justify-content: center;
>            align-items: center;
>        }
>
>        .footer h4 {
>            margin: 0;
>        }
>    </style>
></head>
>
><body>
>    <div class="container">
>        <!-- 导航栏 -->
>        <div class="navbar">
>            <!-- 左侧导航链接 -->
>            <div class="navbar-left">
>                <a href="/">首页</a>
>                <a href="/all_questions">题库</a>
>                <a href="#">竞赛</a>
>                <a href="#">讨论</a>
>                <a href="#">求职</a>
>            </div>
>            <!-- 右侧登录按钮 -->
>            <div class="navbar-right">
>                <a class="login" href="#">登录</a>
>            </div>
>        </div>
>
>        <!-- 题目列表 -->
>        <div class="question_list">
>            <h1>OnlineJudge 题目列表</h1>
>            <table>
>                <tr>
>                    <th class="item">编号</th>
>                    <th class="item">标题</th>
>                    <th class="item">难度</th>
>                </tr>
>                {{#question_list}}
>                <tr>
>                    <td class="item">{{number}}</td>
>                    <td class="item"><a href="/question/{{number}}">{{title}}</a></td>
>                    <td class="item">{{level}}</td>
>                </tr>
>                {{/question_list}}
>            </table>
>        </div>
>
>        <!-- 页脚 -->
>        <div class="footer">
>            <h4>@Xpccccc</h4>
>        </div>
>    </div>
></body>
>
></html>
>
>```
>
>指定题目页：`question.html`
>
>```html
><!DOCTYPE html>
><html lang="en">
>
><head>
>    <meta charset="UTF-8">
>    <meta http-equiv="X-UA-Compatible" content="IE=edge">
>    <meta name="viewport" content="width=device-width, initial-scale=1.0">
>    <title>{{number}}.{{title}}</title>
>    <!-- 引入ACE插件 -->
>    <script src="https://cdnjs.cloudflare.com/ajax/libs/ace/1.2.6/ace.js" type="text/javascript" charset="utf-8"></script>
>    <script src="https://cdnjs.cloudflare.com/ajax/libs/ace/1.2.6/ext-language_tools.js" type="text/javascript" charset="utf-8"></script>
>    <script src="http://code.jquery.com/jquery-2.1.1.min.js"></script>
>
>    <style>
>        /* 清除默认样式 */
>        * {
>            margin: 0;
>            padding: 0;
>            box-sizing: border-box;
>        }
>
>        html,
>        body {
>            width: 100%;
>            height: 100%;
>            font-family: 'Arial', sans-serif;
>            background-color: #f4f4f4;
>        }
>
>        .container {
>            display: flex;
>            flex-direction: column;
>            min-height: 100vh;
>        }
>
>        /* 导航栏样式 */
>        .navbar {
>            display: flex;
>            justify-content: space-between;
>            align-items: center;
>            background-color: #333;
>            padding: 0 20px;
>            height: 60px;
>        }
>
>        /* 左侧导航菜单 */
>        .navbar-left {
>            display: flex;
>        }
>
>        .navbar-left a {
>            color: white;
>            font-size: 18px;
>            text-decoration: none;
>            padding: 14px 20px;
>            transition: background-color 0.3s ease;
>        }
>
>        .navbar-left a:hover {
>            background-color: #4CAF50;
>            border-radius: 4px;
>        }
>
>        /* 右侧登录按钮 */
>        .navbar-right .login {
>            color: white;
>            font-size: 18px;
>            text-decoration: none;
>            padding: 14px 20px;
>            background-color: #4CAF50;
>            border-radius: 4px;
>            transition: background-color 0.3s ease;
>        }
>
>        .navbar-right .login:hover {
>            background-color: #45a049;
>        }
>
>        /* 左右呈现，题目描述和预设代码 */
>        .part1 {
>            display: flex;
>            flex: 1;
>            margin-top: 10px;
>            padding: 20px;
>        }
>
>        .left_desc {
>            flex: 1;
>            padding: 20px;
>            background-color: #ecf0f1;
>            overflow-y: auto;
>        }
>
>        .left_desc h3 {
>            font-size: 22px;
>            color: #2c3e50;
>            margin-bottom: 15px;
>        }
>
>        .left_desc pre {
>            font-size: 16px;
>            line-height: 1.5;
>            white-space: pre-wrap;
>        }
>
>        .right_code {
>            flex: 1;
>            padding: 20px;
>            background-color: #ecf0f1;
>        }
>
>        .right_code .ace_editor {
>            width: 100%;
>            height: 100%;
>            border: 1px solid #bdc3c7;
>            border-radius: 4px;
>        }
>
>        /* 提交代码和显示结果 */
>        .part2 {
>            display: flex;
>            justify-content: space-between;
>            align-items: center;
>            padding: 15px 20px;
>            background-color: #f4f4f4;
>        }
>
>        .result {
>            flex: 1;
>            margin-right: 20px;
>        }
>
>        .btn-submit {
>            width: 120px;
>            height: 50px;
>            background-color: #26bb9c;
>            color: white;
>            border: none;
>            border-radius: 5px;
>            font-size: 18px;
>            cursor: pointer;
>            transition: background-color 0.3s;
>        }
>
>        .btn-submit:hover {
>            background-color: #1abc9c;
>        }
>
>        .result pre {
>            font-size: 16px;
>            margin-top: 10px;
>            white-space: pre-wrap;
>            word-wrap: break-word;
>        }
>    </style>
></head>
>
><body>
>    <div class="container">
>        <!-- 导航栏 -->
>        <div class="navbar">
>            <!-- 左侧导航链接 -->
>            <div class="navbar-left">
>                <a href="/">首页</a>
>                <a href="/all_questions">题库</a>
>                <a href="#">竞赛</a>
>                <a href="#">讨论</a>
>                <a href="#">求职</a>
>            </div>
>            <!-- 右侧登录按钮 -->
>            <div class="navbar-right">
>                <a class="login" href="#">登录</a>
>            </div>
>        </div>
>
>        <!-- 左右呈现，题目描述和预设代码 -->
>        <div class="part1">
>            <div class="left_desc">
>                <h3><span id="number">{{number}}</span>. {{title}} - {{level}}</h3>
>                <pre>{{desc}}</pre>
>            </div>
>            <div class="right_code">
>                <pre id="code" class="ace_editor"><textarea class="ace_text-input">{{pre_code}}</textarea></pre>
>            </div>
>        </div>
>
>        <!-- 提交并显示结果 -->
>        <div class="part2">
>            <div class="result"></div>
>            <button class="btn-submit" onclick="submit()">提交代码</button>
>        </div>
>    </div>
>
>    <script>
>        var editor = ace.edit("code");
>        editor.setTheme("ace/theme/monokai");
>        editor.session.setMode("ace/mode/c_cpp");
>        editor.setFontSize(16);
>        editor.getSession().setTabSize(4);
>        editor.setReadOnly(false);
>
>        ace.require("ace/ext/language_tools");
>        editor.setOptions({
>            enableBasicAutocompletion: true,
>            enableSnippets: true,
>            enableLiveAutocompletion: true
>        });
>
>        function submit() {
>            var code = editor.getSession().getValue();
>            var number = $("#number").text();
>            var judge_url = "/judge/" + number;
>
>            $.ajax({
>                method: 'POST',
>                url: judge_url,
>                dataType: 'json',
>                contentType: 'application/json;charset=utf-8',
>                data: JSON.stringify({
>                    'code': code,
>                    'input': ''
>                }),
>                success: function (data) {
>                    show_result(data);
>                }
>            });
>
>            function show_result(data) {
>                var result_div = $(".container .part2 .result");
>                result_div.empty();
>
>                var reason_label = $("<p>", {
>                    text: data.reason
>                });
>                reason_label.appendTo(result_div);
>
>                if (data.status == 0) {
>                    var stdout_label = $("<pre>", {
>                        text: data.stdout
>                    });
>                    var stderr_label = $("<pre>", {
>                        text: data.stderr
>                    });
>
>                    stdout_label.appendTo(result_div);
>                    stderr_label.appendTo(result_div);
>                }
>            }
>        }
>    </script>
></body>
>
></html>
>
>```
>
>指定题目页的代码框使用的是ACE插件。其中涉及到和后端交互的功能：主要是下面部分
>
>```html
>function submit() {
>            var code = editor.getSession().getValue(); <!-- 获取页面提交后的代码内容 -->
>            var number = $("#number").text();
>            var judge_url = "/judge/" + number;
>
>            $.ajax({									<!-- 写到json串中 -->
>                method: 'POST',
>                url: judge_url,
>                dataType: 'json',
>                contentType: 'application/json;charset=utf-8',
>                data: JSON.stringify({
>                    'code': code,
>                    'input': ''
>                }),
>                success: function (data) { 				<!-- 发起http请求 -->
>                    show_result(data);
>                }
>            });
>
>            function show_result(data) {
>                var result_div = $(".container .part2 .result");
>                result_div.empty();
>
>                var reason_label = $("<p>", {
>                    text: data.reason
>                });
>                reason_label.appendTo(result_div);
>
>                if (data.status == 0) {
>                    var stdout_label = $("<pre>", {
>                        text: data.stdout
>                    });
>                    var stderr_label = $("<pre>", {
>                        text: data.stderr
>                    });
>
>                    stdout_label.appendTo(result_div);
>                    stderr_label.appendTo(result_div);
>                }
>            }
>        }
>```

----



## 7、发布项目

>使用顶层Makefile来操作所有能用到的make命令
>
>```makefile
>.PHONY:all
>all:
>	@cd compile_server;\
>	make;\
>	cd -;\
>	cd oj_server;\
>	make;\
>	cd -;
>
>.PHONY:output
>output:
>	@mkdir -p output/compile_server;\
>	mkdir -p output/oj_server;\
>	cp -rf compile_server/compile_server output/compile_server;\
>	cp -rf compile_server/tmp output/compile_server;\
>	cp -rf oj_server/conf output/oj_server;\
>	cp -rf oj_server/questions output/oj_server;\
>	cp -rf oj_server/template_html output/oj_server;\
>	cp -rf oj_server/wwwroot output/oj_server;\
>	cp -rf oj_server/oj_server output/oj_server;\
>
>.PHONY:clean
>clean:
>	@cd compile_server;\
>	make clean;\
>	cd -;\
>	cd oj_server;\
>	make clean;\
>	cd -;
>	rm -rf output
>```
>
>其中使用命令 `make out `会生成一个没有源代码但可以直接运行的项目。

----



## 8、项目整体效果演示

>![](https://raw.githubusercontent.com/Xpccccc/PicGo/main/data202409101627557.gif)

---



