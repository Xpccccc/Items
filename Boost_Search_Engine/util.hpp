#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "cppjieba/Jieba.hpp"

namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string &path, std::string *out)
        {
            std::ifstream in(path);
            if (!in.is_open())
            {
                std::cout << "open " << path << " file error" << std::endl;
                return false;
            }
            std::string line;
            while (std::getline(in, line))
            {
                (*out) += line;
            }
            in.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        static bool CutString(const std::string &line, std::vector<std::string> *out, const std::string &sep)
        {
            // 使用boost切分split
            boost::split(*out, line, boost::is_any_of(sep), boost::algorithm::token_compress_on);
            return true;
        }
    };

    const char *const DICT_PATH = "./dict/jieba.dict.utf8";
    const char *const HMM_PATH = "./dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "./dict/user.dict.utf8";
    const char *const IDF_PATH = "./dict/idf.utf8";
    const char *const STOP_WORD_PATH = "./dict/stop_words.utf8";
    class JiebaUtil
    {
    private:
        // static cppjieba::Jieba jieba;
        cppjieba::Jieba jieba;
        std::unordered_map<std::string, bool> stop_words;
        JiebaUtil() : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH) {}
        JiebaUtil(const JiebaUtil &) = delete;
        JiebaUtil &operator=(const JiebaUtil &) = delete;
        static JiebaUtil *instance;

    public:
        static JiebaUtil *get_instance()
        {
            std::mutex mtx;
            if (instance == nullptr)
            {
                mtx.lock();
                if (instance == nullptr)
                {
                    instance = new JiebaUtil();
                    instance->InitJiebaUtil();
                }
                mtx.unlock();
            }

            return instance;
        }

        void InitJiebaUtil()
        {
            std::ifstream in(STOP_WORD_PATH);
            if (!in.is_open())
            {
                std::cout << "打开文件失败" << std::endl;
                return;
            }
            std::string line;
            while (std::getline(in, line))
            {
                stop_words.insert({line, true});
            }
            in.close();
        }

        void CutStringForSearchHelper(const std::string &str, std::vector<std::string> *out)
        {
            jieba.CutForSearch(str, *out);
            for (auto iter = out->begin(); iter != out->end();)
            {
                auto it = stop_words.find(*iter); // 看有没有暂停词
                if (it != stop_words.end())
                {
                    // 去掉暂停词
                    iter = out->erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

    public:
        // static void CutStringForSearch(const std::string &str, std::vector<std::string> *out)
        static void CutStringForSearch(const std::string &str, std::vector<std::string> *out)
        {
            // jieba.CutForSearch(str, *out);
            ns_util::JiebaUtil::get_instance()->CutStringForSearchHelper(str, out);
        }
    };

    // cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);
    JiebaUtil *JiebaUtil::instance = nullptr;
}