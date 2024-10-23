#include "searcher.hpp"

const std::string path = "data/raw_html/raw.txt";

int main()
{
    // 创建searcher对象
    ns_searcher::Searcher *searcher = new ns_searcher::Searcher();

    // 初始化
    searcher->InitSearch(path);

    std::string query;
    std::string out_json;
    while (true)
    {
        std::cout << "please enter searcher words # ";
        std::cin >> query;
        searcher->Search(query, &out_json);

        std::cout << out_json << std::endl;
    }
}