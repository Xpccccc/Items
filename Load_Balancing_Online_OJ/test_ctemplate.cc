#include <iostream>

#include <ctemplate/template.h>

int main()
{
    std::string in_html = "./test.html";
    std::string value = "哈哈哈哈哈哈哈";

    // 形成数据字典
    ctemplate::TemplateDictionary root("test");

    root.SetValue("key", value);

    // 获取被渲染的网页对象
    ctemplate::Template *tpl = ctemplate::Template::GetTemplate(in_html, ctemplate::DO_NOT_STRIP);

    // 添加字典数据到网页中
    std::string out_html;
    tpl->Expand(&out_html, &root);

    // 完成渲染
    std::cout << out_html << std::endl;
}