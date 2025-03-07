#include "Html_parser_1.h"
#include "DatabaseManager.h"


// 使用示例
int main() {

    
    // 创建数据库管理器
    DatabaseManager dbManager;
    if (!dbManager.open("websites.db")) {
        std::cerr << "无法打开数据库: " << dbManager.getLastError() << std::endl;
        return 1;
    }

    
   std::string html = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>示例网页</title>
        <meta charset="UTF-8">
        <meta name="description" content="这是一个示例网页，用于演示HTML解析器的功能。">
        <meta name="keywords" content="HTML, 解析器, C++, 示例, 数据库">
        <meta name="author" content="示例作者">
        <meta property="og:title" content="示例网页的Open Graph标题">
        <meta property="og:description" content="这是一个示例网页的Open Graph描述。">
        <meta property="article:published_time" content="2025-03-07T12:00:00Z">
    </head>
    <body>
        <header>
            <h1>示例网页标题</h1>
            <nav>
                <ul>
                    <li><a href="/">首页</a></li>
                    <li><a href="/about">关于我们</a></li>
                    <li><a href="/contact">联系我们</a></li>
                </ul>
            </nav>
        </header>
        <main id="content">
            <article>
                <h2>文章标题</h2>
                <p>这是文章的第一段。这段文本包含了一些内容，用于测试HTML解析器的功能。</p>
                <p>第二段包含了一个<a href="https://example.com">链接</a>和一些<strong>加粗文本</strong>。</p>
                <p>这是第三段内容，用于测试HTML解析器的多段文本提取功能。</p>
                <ul>
                    <li>列表项 1</li>
                    <li>列表项 2</li>
                    <li>列表项 3</li>
                </ul>
                <table border="1">
                    <caption>示例表格</caption>
                    <thead>
                        <tr>
                            <th>列1</th>
                            <th>列2</th>
                            <th>列3</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr>
                            <td>数据1</td>
                            <td>数据2</td>
                            <td>数据3</td>
                        </tr>
                        <tr>
                            <td>数据4</td>
                            <td>数据5</td>
                            <td>数据6</td>
                        </tr>
                        <tr>
                            <td>数据7</td>
                            <td>数据8</td>
                            <td>数据9</td>
                        </tr>
                    </tbody>
                </table>
                <h3>子标题</h3>
                <p>这里是一些额外的信息，用于展示如何处理不同的HTML标签。</p>
                <blockquote>
                    这是一个引用块，用来展示如何解析引用文本。
                </blockquote>
                <figure>
                    <img src="image.jpg" alt="示例图片">
                    <figcaption>图1：示例图片的说明文字。</figcaption>
                </figure>
            </article>
        </main>
        <footer>
            <p>版权所有 &copy; 2025 示例公司. 保留所有权利。</p>
        </footer>
    </body>
    </html>
)";

    // 解析
    HTMLParser parser;
    auto root = parser.parse(html);
    // 转换为WebContent
    WebContent wc = parser.extractContent("http://test.com");
    // 保存到数据库
    if(dbManager.saveContent(wc)) {
        std::cout << "html的WebContent成功保存到数据库" << std::endl;
    } else {
        std::cout << "html的WebContent保存到数据库失败" << std::endl;
    }

    std::cout << "当前数据库内容：\n";
    dbManager.printDatabaseContents();

    return 0;

}