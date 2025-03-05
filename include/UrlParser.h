#pragma once

#include <iostream>
#include <string>
#include <regex>
#include <optional>
#include <unordered_map>
#include <stdexcept>

class Url {
public:
    std::string protocol;   // http/https
    std::string domain;     // 域名
    int port;               // 端口（整数类型）
    std::string path;       // 路径
    std::string query;      // 查询参数
    std::string fragment;   // 片段

    // 打印URL详情
    void print() const {
        std::cout << "协议: " << (protocol.empty() ? "未知" : protocol) << std::endl;
        std::cout << "域名: " << domain << std::endl;
        std::cout << "端口: " << port << std::endl;
        std::cout << "路径: " << (path.empty() ? "/" : path) << std::endl;
        std::cout << "查询参数: " << (query.empty() ? "无" : query) << std::endl;
        std::cout << "片段: " << (fragment.empty() ? "无" : fragment) << std::endl;
    }
};

class UrlParser {
public:
    static std::optional<Url> parse(const std::string& url) {
        // 正则表达式解析URL
        std::regex url_pattern(
            "^(https?://)?"           // 可选协议 group(1)
            "([^:/]+)"                // 域名 group(2)
            "(:([0-9]+))?"            // 可选端口 group(4)
            "(/[^?#]*)*"              // 可选路径 group(5)
            "(\\?[^#]*)?"             // 可选查询参数 group(6)
            "(#.*)?$"                 // 可选片段 group(7)
        );


        std::smatch matches;
        if (!std::regex_match(url, matches, url_pattern)) {
            return std::nullopt;
        }

        Url parsed_url;
        parsed_url.protocol = matches[1].str().substr(0, matches[1].str().length() - 3);
        parsed_url.domain = matches[2].str();
        
        // 端口转换为整数
        try {
            parsed_url.port = matches[4].str().empty() 
                ? (parsed_url.protocol == "https" ? 443 : 80)
                : std::stoi(matches[4].str());
        } catch (const std::exception& e) {
            // 转换失败时使用默认端口
            parsed_url.port = (parsed_url.protocol == "https" ? 443 : 80);
        }

        parsed_url.path = matches[5].str().empty() ? "/" : matches[5].str();
        parsed_url.query = matches[7].str();
        parsed_url.fragment = matches[9].str();

        return parsed_url;
    }

    // 解析查询参数
    static std::unordered_map<std::string, std::string> parseQueryParams(const std::string& query) {
        std::unordered_map<std::string, std::string> params;
        
        if (query.empty()) return params;

        std::string query_str = (query[0] == '?') ? query.substr(1) : query;
        std::istringstream iss(query_str);
        std::string param;

        while (std::getline(iss, param, '&')) {
            size_t pos = param.find('=');
            if (pos != std::string::npos) {
                std::string key = param.substr(0, pos);
                std::string value = param.substr(pos + 1);
                params[key] = value;
            }
        }

        return params;
    }
};
