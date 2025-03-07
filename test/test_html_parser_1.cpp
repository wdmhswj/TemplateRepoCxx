#include "Html_parser_1.h"

// 使用示例
int main() {
    HTMLParser parser;
    
    // 解析HTML字符串
    std::string html = R"(
    <!DOCTYPE html>
    <html
    <head>
        <title>测试页面</title>
        <meta charset="UTF-8">
    </head>
    <body>
        <header>
            <h1 id="page-title">这是标题</h1>
            <nav>
                <ul>
                    <li><a href="#">首页</a></li>
                    <li><a href="#">关于</a></li>
                    <li><a href="#">联系我们</a></li>
                </ul>
            </nav>
        </header>
        <main>
            <article>
                <h2>文章标题</h2>
                <p>这是一个段落，包含<a href="https://example.com">链接</a>和<strong>加粗文本</strong>。</p>
                <p>另一个段落。</p>
            </article>
            <aside>
                <h3>侧边栏</h3>
                <p>这是侧边栏内容。</p>
            </aside>
        </main>
        <footer>
            <!-- 这是一个注释 -->
            <p>&copy; 2025 示例网站</p>
        </footer>
    </body>
    </html>
    )";
    
    std::shared_ptr<HTMLParser::HTMLNode> rootNode = parser.parse(html);
    
    // 打印HTML树结构
    std::cout << "HTML树结构：" << std::endl;
    parser.printTree();
    
    // 查找所有段落
    std::cout << "\n查找所有段落：" << std::endl;
    auto paragraphs = parser.querySelector("p");
    std::cout << "找到 " << paragraphs.size() << " 个段落" << std::endl;
    for (std::shared_ptr<HTMLParser::HTMLNode> p : paragraphs) {
        if (!p->children.empty() && p->children[0]->type == NodeType::TEXT) {
            // std::cout << "段落内容: " << p->children[0]->content << std::endl;
            std::cout << "段落内容：";
            // std::cout << p->content << std::endl;
            for(std::shared_ptr<HTMLParser::HTMLNode> c: p->children) {
                std::cout << "\t" << c->content << std::endl;
            }
        }
    }
    
    // 通过ID查找元素
    std::cout << "\n通过ID查找元素：" << std::endl;
    std::shared_ptr<HTMLParser::HTMLNode> title = parser.getElementById("page-title");
    if (title && !title->children.empty() && title->children[0]->type == NodeType::TEXT) {
        std::cout << "页面标题: " << title->children[0]->content << std::endl;
    }
   
    std::cout << "\n通过Tag查找元素：" << std::endl;
    auto lis = parser.querySelector("li");
    std::cout << "找到" << lis.size() << "个 <li> 标签" << std::endl;
    for(auto li: lis) {
        HTMLParser::printNode(li);
    }
    return 0;
}