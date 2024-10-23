#include "httplib.h"
#include "searcher.hpp"

const std::string path = "data/raw_html/raw.txt";

int main()
{
    httplib::Server svr;
    ns_searcher::Searcher searcher;
    searcher.InitSearch(path);

    svr.Get("/s", [&searcher](const httplib::Request &req, httplib::Response &resp)
            { 
                if(!req.has_param("word")){
                    resp.set_content("必须要有搜索关键字","text/plain;charset=utf-8");
                    return ;
                }
                std::string word = req.get_param_value("word");// 获取提交的参数
                std::cout <<"用户正在搜索 "<< word << std::endl;
                std::string out_json;
                searcher.Search(word,&out_json);
                resp.set_content(out_json.c_str(), "application/json;charset=utf-8"); });
    svr.set_base_dir("./wwwroot");
    svr.listen("0.0.0.0", 8081);
    return 0;
}