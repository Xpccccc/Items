#pragma once

#include <algorithm>
#include <jsoncpp/json/json.h>
#include "index.hpp"

namespace ns_searcher
{
    struct InvertedElemPrint
    {
        uint64_t doc_id;
        int weight;                     // 属于同一文档的weight
        std::vector<std::string> words; // 属于同一文档的关键字
        InvertedElemPrint() : doc_id(0), weight(0) {}
    };

    class Searcher
    {
    public:
        Searcher() {}
        ~Searcher() {}

    public:
        // 初始化
        void InitSearch(const std::string &input)
        {
            // 1.获取Index对象
            _index = ns_index::Index::GetInstance();
            std::cout << "获取index单例成功 ... " << std::endl;
            // 2.根据Index对象建立索引
            _index->BulidIndex(input);
            std::cout << "建立正排索引和倒排索引成功 ... " << std::endl;
        }

        // query:关键字查询
        // out_json:返回的json串
        void Search(const std::string &query, std::string *out_json)
        {
            // 1.分词：对query进行分词
            std::vector<std::string> words;
            ns_util::JiebaUtil::CutStringForSearch(query, &words);

            // 空格得去掉
            // std::cout << "debug" << std::endl;

            // for (auto &e : words)
            // {
            //     std::cout << e << std::endl;
            // }

            // 2.触发：根据分词后的各个词，进行index查找
            // ns_index::InvertedList inverted_list_all;                       // 存当前所有的关键字的拉链，拉链依次放在后面
            std::vector<InvertedElemPrint> inverted_list_all;               // 存文档id去重之后要保存的节点
            std::unordered_map<uint64_t, InvertedElemPrint> inverted_print; //  一个文档对应的关键字和权重
            for (auto &word : words)
            {
                // 查找倒排拉链
                boost::to_lower(word);
                ns_index::InvertedList *inverted_list = _index->GetInvertedIndex(word); // 存的InvertElem
                if (nullptr == inverted_list)
                {
                    continue;
                }
                // 找到了倒排拉链
                // 不完美！可能存在关键字在同一文档出现，导致显示多个html
                // inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end()); // 存的InvertElem

                for (auto &elem : *inverted_list)
                {
                    // 把关键字对应的文档放到inverted_print
                    InvertedElemPrint &item = inverted_print[elem.doc_id];
                    // 这里之后，item一定是doc_id相同的节点
                    item.doc_id = elem.doc_id;
                    item.weight += elem.weight; // 同一个doc_id的关键字就权值相加
                    item.words.push_back(elem.word);
                }
            }

            // 把inverted_print的InvertedElemPrint放到inverted_list_all里面
            for (auto &item : inverted_print)
            {
                inverted_list_all.push_back(std::move(item.second));
            }

            // 3.合并排序：汇总查找结果，按照相关性（weight）进行降序排序
            // sort(inverted_list_all.begin(), inverted_list_all.end(), [](const ns_index::InvertElem &e1, const ns_index::InvertElem &e2)
            //      { return e1.weight > e2.weight; });

            sort(inverted_list_all.begin(), inverted_list_all.end(), [](const InvertedElemPrint &e1, const InvertedElemPrint &e2)
                 { return e1.weight > e2.weight; });

            // 4.构建：根据查找出来的结果，构建json串 -- jsoncpp
            Json::Value root;
            for (auto &elem : inverted_list_all) // 已经有序
            {
                // 获取正排索引的文档id
                ns_index::DocInfo *doc = _index->GetForwardIndex(elem.doc_id);
                Json::Value value;
                value["title"] = doc->title;
                value["desc"] = GetDesc(doc->content, elem.words[0]); // GetDesc(doc->content, elem.word); // 只获取摘要
                value["url"] = doc->url;

                // for debug ,for delete
                value["doc_id"] = (int)elem.doc_id;
                value["weight"] = elem.weight;

                root.append(value);
            }
            Json::StyledWriter writer;
            *out_json = writer.write(root);
        }

        std::string GetDesc(const std::string &content, const std::string &word)
        {
            // 找到关键字的左边50字节，右边100字节
            const int prev_len = 50;
            const int next_len = 100;
            // 这里注意，content里面的内容是没有进行转小写的，而word是小写的，因此我们需要对content特殊处理来查找word
            auto iter = std::search(content.begin(), content.end(), word.begin(), word.end(), [](const char c1, const char c2)
                                    { return std::tolower(c1) == std::tolower(c2); });
            if (iter == content.end())
            {
                return "None1";
            }
            int pos = std::distance(content.begin(), iter);
            int start = 0;                // 如果当前关键字前面没有50字节，就用这个作为起始点
            int end = content.size() - 1; // 如果当前关键字后面没有100字节，就用这个作为结束点

            if (pos > start + prev_len)
                start = pos - prev_len;
            if (pos < end - next_len)
                end = pos + next_len;

            // 获取子串
            if (end <= start)
                return "None2";
            std::string ret = content.substr(start, end - start) + " ... ";
            return ret;
        }

    private:
        ns_index::Index *_index; // 供系统进行查找的索引
    };
}