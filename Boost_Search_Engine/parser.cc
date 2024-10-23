#include <iostream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "util.hpp"

typedef struct DocInfo
{
    std::string title;   // 文档标题
    std::string content; // 文档内容
    std::string url;     // 文档在官网的url
} DocInfo_t;

const std::string search_file = "data/input/";
const std::string output = "data/raw_html/raw.txt";

// const &: 输入
// *: 输出
// &: 输入输出

bool EnumFile(const std::string &search_file, std::vector<std::string> *files_list);
bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results);
bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output);

int main()
{
    std::vector<std::string> files_list;
    // 第一步：递归式的把每个html文件名带路径，保存到files_list中，方便后期进行一个一个的文件读取
    if (!EnumFile(search_file, &files_list))
    {
        std::cerr << "enum file name error!" << std::endl;
        return 1;
    }
    std::vector<DocInfo_t> results;
    // 第二步：按照files_list读取每个文件的内容，并进行解析
    if (!ParseHtml(files_list, &results))
    {
        std::cerr << "parse html error!" << std::endl;
        return 2;
    }
    // 第三步：把解析好的文件，写入到output，按照\3作为每个文件的分隔符
    if (!SaveHtml(results, output))
    {
        std::cerr << "save html error!" << std::endl;
        return 3;
    }
    return 0;
}

bool EnumFile(const std::string &search_file, std::vector<std::string> *files_list)
{
    namespace fs = boost::filesystem;
    fs::path root_path(search_file);
    // 判断路径是否存在，不存在就直接退出
    if (!fs::exists(root_path))
    {
        std::cerr << search_file << " not exists!" << std::endl;
        return false;
    }

    // 定义一个空的迭代器，用来进行判断递归结束
    fs::recursive_directory_iterator end;
    for (fs::recursive_directory_iterator iter(root_path); iter != end; ++iter)
    {
        // 判断文件是否是普通文件，html都是普通文件
        if (!fs::is_regular_file(*iter))
        {
            continue;
        }
        // 判断普通文件是不是html后缀
        if (iter->path().extension() != ".html")
        {
            continue;
        }

        // debug
        // std::cout << "debug : " << iter->path().string() << std::endl;

        // 当前路径合法
        // 将所有带路径的html文件放到files_list容器
        files_list->push_back(iter->path().string());
    }
    return true;
}

static bool ParseTitle(const std::string &result, std::string *title)
{
    std::size_t begin = result.find("<title>");
    if (begin == std::string::npos)
    {
        return false;
    }
    std::size_t end = result.find("</title>");
    if (end == std::string::npos)
    {
        return false;
    }
    if (begin > end)
        return false;
    begin += std::string("<title>").size();
    *title = result.substr(begin, end - begin);
    return true;
}

static bool ParseContent(const std::string &result, std::string *content)
{
    // 去标签，基于一个简单的状态机
    enum status
    {
        LABEL,
        CONTENT
    };

    enum status s = LABEL;
    for (char c : result)
    {
        switch (s)
        {
        case LABEL:
            if (c == '>')
                s = CONTENT;
            break;
        case CONTENT:
            if (c == '<')
                s = LABEL;
            else
            {
                // 我们不想保留原始文件中的\n，因为我们想用\n作为html解析之后文本的分隔符
                if (c == '\n')
                    c = ' ';
                (*content) += c;
            }
            break;
        default:
            break;
        }
    }
    return true;
}

static bool ParseUrl(const std::string &file_path, std::string *url)
{
    std::string url_head = "https://www.boost.org/doc/libs/1_86_0/doc/html/";
    std::string url_tail = file_path.substr(search_file.size()); // 文件路径去掉 data/input/ 前缀，就剩下 X.html
    *url = url_head + url_tail;
    return true;
}

void ShowDoc(const DocInfo_t &doc)
{
    std::cout << "Title: " << doc.title << std::endl;
    std::cout << "Content: " << doc.content << std::endl;
    std::cout << "Url: " << doc.url << std::endl;
}

bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results)
{
    for (auto &file : files_list)
    {
        // 1.读指定文件
        std::string result;
        if (!ns_util::FileUtil::ReadFile(file, &result))
        {
            continue;
        }
        // 2.获取指定文件title
        DocInfo_t doc;
        if (!ParseTitle(result, &doc.title))
        {
            continue;
        }
        // 3.获取指定文件content
        if (!ParseContent(result, &doc.content))
        {
            continue;
        }
        // 4.构建指定文件url
        if (!ParseUrl(file, &doc.url))
        {
            continue;
        }
        // 解析完成，放到results里
        results->push_back(std::move(doc)); // 有拷贝可以优化

        // 测试功能
        // ShowDoc(doc);
        // break;
    }
    return true;
}

bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output)
{
#define SEP "\3"
    // 二进制形式代开文件
    std::ofstream out(output, std::ios::out | std::ios::binary);
    if (!out.is_open())
    {
        std::cerr << "open " << output << " error !" << std::endl;
        return false;
    }

    // title\3content\3url\ntitle\3content\3url\ntitle\3content\3url\n
    for (auto &item : results)
    {
        std::string out_result = item.title;
        out_result += SEP;
        out_result += item.content;
        out_result += SEP;
        out_result += item.url;
        out_result += '\n';
        out.write(out_result.c_str(), out_result.size());
    }
    return true;
}