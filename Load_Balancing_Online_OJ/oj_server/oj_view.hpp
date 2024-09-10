#pragma once

#include <ctemplate/template.h>

#include <string>
#include <vector>

// #include "oj_model.hpp"
#include "oj_model_MySQL.hpp"

namespace ns_view
{
    using namespace ns_model;

    const std::string draw_path = "./template_html/";
    class View
    {
    public:
        void DrawAllQuestions(std::vector<Question> &all, std::string *html)
        {
            std::string in_html = draw_path + "all_questions.html";
            // 形成数据字典
            ctemplate::TemplateDictionary root("all");

            for (const auto &q : all)
            {
                // 形成子数据字典
                ctemplate::TemplateDictionary *sub = root.AddSectionDictionary("question_list");
                sub->SetValue("number", q.number);
                sub->SetValue("title", q.title);
                sub->SetValue("level", q.level);
            }

            // 获取被渲染的网页对象
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(in_html, ctemplate::DO_NOT_STRIP);

            // 添加字典数据到网页中
            tpl->Expand(html, &root);
        }

        void DrawOneQuestion(const Question &q, std::string *html)
        {
            std::string in_html = draw_path + "question.html";

            ctemplate::TemplateDictionary root("one");

            root.SetValue("number", q.number);
            root.SetValue("title", q.title);
            root.SetValue("level", q.level);

            root.SetValue("desc", q.desc);
            root.SetValue("pre_code", q.header);

            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(in_html, ctemplate::DO_NOT_STRIP);

            tpl->Expand(html, &root);
        }
    };
}
