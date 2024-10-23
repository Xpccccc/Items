#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include "util.hpp"

namespace ns_index
{
    struct DocInfo
    {
        std::string title;    // 文档标题
        std::string content;  // 文档内容
        std::string url;      // 官方对应的网址
        std::uint64_t doc_id; // 文档id
    };

    struct InvertElem
    {
        std::uint64_t doc_id; // 文档id
        std::string word;     // 关键字
        int weight;           // 权重
    };

    // 倒排拉链
    typedef std::vector<InvertElem> InvertedList;
    class Index
    {
    private:
        Index() {}
        Index(const Index &) = delete;
        Index &operator=(const Index &) = delete;
        ~Index() {}

    private:
        static std::mutex mtx;
        static Index *instance;

    public:
        // 单例模式
        static Index *GetInstance()
        {
            if (instance == nullptr)
            {
                mtx.lock();
                if (instance == nullptr)
                {
                    instance = new Index();
                }
                mtx.unlock();
            }

            return instance;
        }

    public:
        // 根据去标签的文件/data/raw_html/raw.txt，建立正排索引和倒排索引
        // 根据doc_id找到文档内容
        bool BulidIndex(const std::string &path)
        {
            std::ifstream in(path, std::ios::in | std::ios::binary);
            if (!in.is_open())
            {
                std::cerr << "open " << path << " file error" << std::endl;
                return false;
            }
            std::string line;
            int count = 0;
            while (std::getline(in, line))
            {
                // 构建正排索引
                DocInfo *doc = BulidForwardIndex(line);
                if (doc == nullptr)
                {
                    continue;
                }
                // 构建倒排排索引
                if (!BuildInvertedIndex(*doc))
                {
                    continue;
                }

                ++count;
                if (count % 50 == 0)
                {
                    std::cout << "建立第 " << count << " 个文档索引成功" << std::endl;
                }
            }
            in.close();
            return true;
        }
        DocInfo *GetForwardIndex(uint64_t doc_id)
        {
            if (doc_id > _forward_index.size())
            {
                std::cerr << "doc_id out of range!" << std::endl;
                return nullptr;
            }
            return &_forward_index[doc_id];
        }
        // 根据关键字找到文档id，即获得倒排拉链
        InvertedList *GetInvertedIndex(const std::string &word)
        {
            auto iter = _inverted_index.find(word);
            if (iter == _inverted_index.end())
            {
                std::cerr << word << " not find" << std::endl;
                return nullptr;
            }
            return &iter->second;
        }

    private:
        DocInfo *BulidForwardIndex(const std::string &line)
        {
            // 1.解析line，进行切分字符串
            std::vector<std::string> results;
            const std::string sep = "\3"; // 行内分隔符
            ns_util::StringUtil::CutString(line, &results, sep);
            if (results.size() != 3)
                return nullptr;
            // 2.填充DocInfo并插入到正排索引列表
            DocInfo doc;
            doc.title = results[0];
            doc.content = results[1];
            doc.url = results[2];
            doc.doc_id = _forward_index.size();
            _forward_index.push_back(std::move(doc));
            return &_forward_index.back(); // 当前最后一个元素的地址
        }

        // 注意：这里是某一个文档的的
        bool BuildInvertedIndex(const DocInfo &doc)
        {
            // DocInfo【title，content，url，doc_id】
            // 分词
            // 词频统计
            struct word_cnt
            {
                int title_cnt;
                int content_cnt;
                word_cnt() : title_cnt(0), content_cnt(0) {}
            };

            // 关键字的词频映射
            std::unordered_map<std::string, word_cnt> word_weight;

            // 标题的分词
            std::vector<std::string> title_words;
            ns_util::JiebaUtil::CutStringForSearch(doc.title, &title_words); // 标题分词
            for (auto s : title_words)                                       // 这里不要引用，防止修改了原字符串
            {
                boost::to_lower(s);         // 全部转成小写，不区分大小写
                word_weight[s].title_cnt++; // 插入到映射表
            }

            // 内容分词
            std::vector<std::string> content_words;
            ns_util::JiebaUtil::CutStringForSearch(doc.content, &content_words);
            for (auto s : content_words) // 这里不要引用，防止修改了原字符串
            {
                boost::to_lower(s);
                word_weight[s].content_cnt++;
            }

            // 已经建立完映射表
            // 现在建立倒排拉链
#define X 10
#define Y 1
            for (auto &word_pair : word_weight)
            {
                InvertElem elem;
                elem.doc_id = doc.doc_id; // 当前文档的id
                elem.word = word_pair.first;
                elem.weight = X * word_pair.second.title_cnt + Y * word_pair.second.content_cnt; // 相关性
                InvertedList &inverted_list = _inverted_index[word_pair.first];                  // 找到倒排拉链，再在这个倒排拉链插入元素
                inverted_list.emplace_back(std::move(elem));
            }
            return true;
        }

    private:
        std::vector<DocInfo> _forward_index;                           // 正排索引
        std::unordered_map<std::string, InvertedList> _inverted_index; // 倒排索引
    };

    Index *Index::instance = nullptr;
    std::mutex Index::mtx;
}