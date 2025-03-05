#include <iostream>
#include <string>
#include <regex>

bool isValidUrl(const std::string& url) {
    // 更全面的URL正则表达式
    std::regex url_pattern(
        "^(https?://)"                   // 协议
        "(www\\.?)?"                     // 可选的www
        "([a-zA-Z0-9-]+\\.)*"            // 子域名
        "[a-zA-Z0-9-]+"                  // 主域名
        "(\\.[a-zA-Z]{2,})?"             // 顶级域名
        "(/[\\w\\-\\.~:/?#\\[\\]@!$&'()*+,;=]*)?$" // 路径和查询参数
    );


    return std::regex_match(url, url_pattern);
}

int main() {
    std::string input;
    
    // 使用getline读取整行
    std::cout << "请输入完整的URL: ";
    std::getline(std::cin, input);

    // 去除首尾空白
    input.erase(0, input.find_first_not_of(" \t"));
    input.erase(input.find_last_not_of(" \t") + 1);

    if (input.empty()) {
        std::cerr << "错误：未输入URL" << std::endl;
        return 1;
    }
    std::cout << "input = " << input << std::endl;
    try {
        if (isValidUrl(input)) {
            std::cout << "有效的URL：" << input << std::endl;
            
            // 额外的协议检测
            if (input.substr(0, 5) == "https") {
                std::cout << "使用安全协议 HTTPS" << std::endl;
            } else if (input.substr(0, 4) == "http") {
                std::cout << "使用标准协议 HTTP" << std::endl;
            }
        } else {
            std::cout << "无效的URL：" << input << std::endl;
        }
    }
    catch (const std::regex_error& e) {
        std::cerr << "正则表达式错误：" << e.what() << std::endl;
        return 1;
    }

    return 0;
}