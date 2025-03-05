#include "Database.h"


int main() {
    Database db("crawler.db");

    // ✅ 存储新页面数据
    std::string url = "https://example.com";
    int status_code = 200;
    std::string content = "<html><body>Example Page Content</body></html>";

    if (db.storePage(url, status_code, content)) {
        std::cout << "✅ Page stored successfully!\n";
    } else {
        std::cout << "❌ Failed to store page.\n";
    }

    // ✅ 读取并打印数据库中的所有数据
    db.fetchAllPages();

    return 0;
}
