#include "UrlParser.h"


int main() {
    // 测试用例
    std::vector<std::string> test_urls = {
        "http://www.example.com",
        "https://example.com:8080/path/to/page?name=value#section",
        "http://example.com/path?key1=value1&key2=value2",
        "https://subdomain.example.com:443/path",
        "http://yhirose.github.io",
        "https://baidu.com"
    };

    for (const auto& url_str : test_urls) {
        std::cout << "解析URL: " << url_str << std::endl;
        
        auto parsed_url = UrlParser::parse(url_str);
        
        if (parsed_url) {
            parsed_url->print();
            
            // 解析查询参数
            if (!parsed_url->query.empty()) {
                std::cout << "查询参数：" << std::endl;
                auto params = UrlParser::parseQueryParams(parsed_url->query);
                for (const auto& [key, value] : params) {
                    std::cout << key << " = " << value << std::endl;
                }
            }
            
            std::cout << "------------------------" << std::endl;
        } else {
            std::cout << "无法解析URL" << std::endl;
        }
    }

    return 0;
}