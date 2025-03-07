#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <memory>
#include <sqlite3.h> // 使用SQLite作为示例数据库
#include "Structs.h"





// HTML解析器类
class HTMLParser {
public:
    // HTML节点结构
    struct HTMLNode {
        NodeType type;
        std::string name;
        std::string content;
        std::map<std::string, std::string> attributes;
        std::weak_ptr<HTMLNode> parent; // 避免循环引用
        std::vector<std::shared_ptr<HTMLNode>> children;
        
        HTMLNode(NodeType t, std::string c="", std::string n="")
            : type(t)
            , name(n)
            , content(c) {}
        ~HTMLNode() = default;
    };

private:
    std::string html;
    size_t pos = 0;
    std::shared_ptr<HTMLNode> root;
    
    // 跳过空白字符
    void skipWhitespace() {
        while (pos < html.length() && isspace(html[pos])) {
            pos++;
        }
    }
    
    // 解析标签名
    std::string parseTagName() {
        std::string name;
        while (pos < html.length() && !isspace(html[pos]) && html[pos] != '>' && html[pos] != '/') {
            name += html[pos++];
        }
        return name;
    }
    
    // 解析属性
    std::map<std::string, std::string> parseAttributes() {
        std::map<std::string, std::string> attrs;
        
        while (pos < html.length() && html[pos] != '>' && html[pos] != '/') {
            skipWhitespace();
            
            // 如果到达标签结束，退出
            if (html[pos] == '>' || html[pos] == '/') {
                break;
            }
            
            // 解析属性名
            std::string name;
            while (pos < html.length() && !isspace(html[pos]) && html[pos] != '=' && html[pos] != '>' && html[pos] != '/') {
                name += html[pos++];
            }
            
            skipWhitespace();
            
            // 检查是否有等号
            if (pos < html.length() && html[pos] == '=') {
                pos++; // 跳过等号
                skipWhitespace();
                
                // 解析属性值
                std::string value;
                char quote = 0;
                
                if (pos < html.length() && (html[pos] == '"' || html[pos] == '\'')) {
                    quote = html[pos++]; // 记录引号类型并跳过
                }
                
                while (pos < html.length()) {
                    if (quote && html[pos] == quote) {
                        pos++; // 跳过结束引号
                        break;
                    } else if (!quote && (isspace(html[pos]) || html[pos] == '>' || html[pos] == '/')) {
                        break;
                    }
                    value += html[pos++];
                }
                
                attrs[name] = value;
            } else if (!name.empty()) {
                // 无值属性，如checked, disabled等
                attrs[name] = "";
            }
        }
        
        return attrs;
    }
    
    // 解析注释
    std::shared_ptr<HTMLNode> parseComment() {
        // 跳过 <!--
        pos += 4;
        
        std::string content;
        while (pos + 2 < html.length()) {
            if (html.substr(pos, 3) == "-->") {
                pos += 3;
                break;
            }
            content += html[pos++];
        }
        return std::make_shared<HTMLNode>(NodeType::COMMENT, content);
    }
    
    // 解析DOCTYPE
    std::shared_ptr<HTMLNode> parseDoctype() {
        // 跳过 <!DOCTYPE
        size_t start = pos;
        while (pos < html.length() && html[pos] != '>') {
            pos++;
        }
        if (pos < html.length()) {
            pos++; // 跳过 >
        }
        
        // std::shared_ptr<HTMLNode> node = new HTMLNode(NodeType::DOCTYPE);
        auto node = std::make_shared<HTMLNode>(NodeType::DOCTYPE);
        node->content = html.substr(start, pos - start);
        return node;
    }
    
    // 解析元素
    std::shared_ptr<HTMLNode> parseElement() {
        // 跳过 <
        pos++;
        
        // 检查是否为结束标签
        if (html[pos] == '/') {
            // 这是结束标签，我们在parseNodes中处理
            return nullptr;
        }
        
        // 解析标签名
        std::string tagName = parseTagName();
        std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);
        
        // 创建元素节点
        auto node = std::make_shared<HTMLNode>(NodeType::ELEMENT);
        node->name = tagName;
        
        // 解析属性
        node->attributes = parseAttributes();
        
        skipWhitespace();
        
        // 检查是否为自闭合标签
        bool selfClosing = false;
        if (pos < html.length() && html[pos] == '/') {
            selfClosing = true;
            pos++;
        }
        
        // 跳过结束括号 >
        if (pos < html.length() && html[pos] == '>') {
            pos++;
        }
        
        // 如果不是自闭合标签，解析子节点
        if (!selfClosing && !isVoidElement(tagName)) {
            parseNodes(node);
        }
        
        return node;
    }
    
    // 解析文本内容
    std::shared_ptr<HTMLNode> parseText() {
        std::string content;
        while (pos < html.length() && html[pos] != '<') {
            content += html[pos++];
        }
        
        // 忽略纯空白的文本节点
        if (std::all_of(content.begin(), content.end(), isspace)) {
            return nullptr;
        }
        
        return std::make_shared<HTMLNode>(NodeType::TEXT, content);
    }
    
    // 检查是否为void元素（没有结束标签的元素）
    bool isVoidElement(const std::string& tagName) {
        static const std::vector<std::string> voidElements = {
            "area", "base", "br", "col", "embed", "hr", "img", "input",
            "link", "meta", "param", "source", "track", "wbr"
        };
        return std::find(voidElements.begin(), voidElements.end(), tagName) != voidElements.end();
    }
    
    // 解析子节点
    void parseNodes(std::shared_ptr<HTMLNode> parent) {
        while (pos < html.length()) {
            // 检查是否为结束标签
            if (pos + 1 < html.length() && html[pos] == '<' && html[pos + 1] == '/') {
                // 跳过 </
                pos += 2;
                
                // 解析标签名
                std::string tagName = parseTagName();
                std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);
                
                // 跳过结束标签的剩余部分
                while (pos < html.length() && html[pos] != '>') {
                    pos++;
                }
                if (pos < html.length()) {
                    pos++; // 跳过 >
                }
                
                // 如果找到匹配的结束标签，退出
                if (tagName == parent->name) {
                    break;
                }
            }
            
            // 解析子节点
            auto node = parseNode();
            if (node) {
                node->parent = parent;
                parent->children.push_back(node);
            }
        }
    }
    
    // 解析节点
    std::shared_ptr<HTMLNode> parseNode() {
        skipWhitespace();
        
        if (pos >= html.length()) {
            return nullptr;
        }
        
        if (html[pos] == '<') {
            // 检查是否为注释
            if (pos + 3 < html.length() && html.substr(pos, 4) == "<!--") {
                return parseComment();
            }
            // 检查是否为DOCTYPE
            else if (pos + 8 < html.length() && html.substr(pos, 9) == "<!DOCTYPE") {
                return parseDoctype();
            }
            // 检查是否为结束标签
            else if (pos + 1 < html.length() && html[pos + 1] == '/') {
                // 结束标签在parseNodes中处理
                return nullptr;
            }
            // 否则是普通元素
            else {
                return parseElement();
            }
        } else {
            // 文本节点
            return parseText();
        }
    }
    
public:
    HTMLParser() : root(nullptr) {}
    
    ~HTMLParser() = default;
    
    // 解析HTML字符串
    std::shared_ptr<HTMLNode> parse(const std::string& htmlContent) {
        html = htmlContent;
        pos = 0;
        
        // 创建根节点
        root = std::make_shared<HTMLNode>(NodeType::ELEMENT);
        root->name = "document";

        
        // 解析所有子节点
        while (pos < html.length()) {
            auto node = parseNode();
            if (node) {
                node->parent = root;
                root->children.push_back(node);
            }
        }
        
        return root;
    }
    
    // 从文件中解析HTML
    std::shared_ptr<HTMLNode> parseFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开文件：" << filename << std::endl;
            return nullptr;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        return parse(content);
    }
    
    // 打印HTML树（用于调试）
    void printTree(int depth = 0) {
        printNode(root);
    }
    
    // 打印node节点为根树（用于调试）
    void static printNode(std::shared_ptr<HTMLNode> node, int depth = 0) {
        if (!node) {
            return;
        }
        
        std::string indent(depth * 2, ' ');
        
        switch (node->type) {
            case NodeType::ELEMENT:
                std::cout << indent << "<" << node->name;
                for (const auto& attr : node->attributes) {
                    std::cout << " " << attr.first;
                    if (!attr.second.empty()) {
                        std::cout << "=\"" << attr.second << "\"";
                    }
                }
                std::cout << ">" << std::endl;
                break;
            case NodeType::TEXT:
                std::cout << indent << "TEXT: " << node->content << std::endl;
                break;
            case NodeType::COMMENT:
                std::cout << indent << "COMMENT: " << node->content << std::endl;
                break;
            case NodeType::DOCTYPE:
                std::cout << indent << "DOCTYPE: " << node->content << std::endl;
                break;
        }
        
        for (auto child : node->children) {
            printNode(child, depth + 1);
        }
        
        if (node->type == NodeType::ELEMENT) {
            std::cout << indent << "</" << node->name << ">" << std::endl;
        }
    }

    // 使用CSS选择器查找元素（简化版本）
    std::vector<std::shared_ptr<HTMLNode>> querySelector(const std::string& selector) {
        std::vector<std::shared_ptr<HTMLNode>> result;
        if (!root) {
            return result;
        }
        
        // 简单实现：仅支持标签名选择器
        queryByTagName(root, selector, result);
        return result;
    }
    
    // 通过ID查找元素
    std::shared_ptr<HTMLNode> getElementById(const std::string& id) {
        if (!root) {
            return nullptr;
        }
        
        return findElementById(root, id);
    }
    
    // 查找指定标签的所有元素
    std::vector<std::shared_ptr<HTMLNode>> getElementsByTagName(std::shared_ptr<HTMLNode> node, const std::string& tagName) {
        std::vector<std::shared_ptr<HTMLNode>> result;
        if (!node) {
            return result;
        }
        
        if (node->type == NodeType::ELEMENT && node->name == tagName) {
            result.push_back(node);
        }
        
        for (std::shared_ptr<HTMLNode> child : node->children) {
            auto childResults = getElementsByTagName(child, tagName);
            result.insert(result.end(), childResults.begin(), childResults.end());
        }
        
        return result;
    }   
    // 获取节点内的所有文本内容
    std::string getTextContent(std::shared_ptr<HTMLNode> node) {
        if (!node) {
            return "";
        }
        
        if (node->type == NodeType::TEXT) {
            return node->content;
        }
        
        std::string result;
        for (auto child : node->children) {
            result += getTextContent(child);
            // 如果是块级元素，添加换行符
            if (child->type == NodeType::ELEMENT && 
                (child->name == "p" || child->name == "div" || 
                 child->name == "h1" || child->name == "h2" || 
                 child->name == "h3" || child->name == "h4" || 
                 child->name == "h5" || child->name == "h6" || 
                 child->name == "li" || child->name == "br")) {
                result += "\n";
            }
        }
        
        return result;
    }

        // 提取网页内容，转换为结构化数据
    WebContent extractContent(const std::string& url) {
        WebContent content;
        content.url = url;
        
        if (!root) {
            std::cerr << "HTML尚未解析，无法提取内容" << std::endl;
            return content;
        }
        
        // 提取标题
        auto titleElements = getElementsByTagName(root, "title");
        if (!titleElements.empty()) {
            content.title = getTextContent(titleElements[0]);
        }
        
        // 提取元数据
        auto metaElements = getElementsByTagName(root, "meta");
        for (std::shared_ptr<HTMLNode> meta : metaElements) {
            std::string name = meta->attributes["name"];
            std::string property = meta->attributes["property"];
            std::string content_ = meta->attributes["content"];
            
            if (!name.empty() && !content_.empty()) {
                content.metadata[name] = content_;
                
                // 提取特定元数据
                if (name == "description") {
                    content.description = content_;
                } else if (name == "keywords") {
                    // 分割关键词
                    std::istringstream iss(content_);
                    std::string keyword;
                    while (std::getline(iss, keyword, ',')) {
                        // 去除两端空格
                        keyword.erase(0, keyword.find_first_not_of(" \t\n\r\f\v"));
                        keyword.erase(keyword.find_last_not_of(" \t\n\r\f\v") + 1);
                        if (!keyword.empty()) {
                            content.keywords.push_back(keyword);
                        }
                    }
                } else if (name == "author") {
                    content.author = content_;
                } else if (!property.empty() && !content_.empty()) {
                    content.metadata[property] = content_;
                    
                    // 处理Open Graph和其他元数据
                    if (property == "og:title" && content.title.empty()) {
                        content.title = content_;
                    } else if (property == "og:description" && content.description.empty()) {
                        content.description = content_;
                    } else if (property == "article:published_time") {
                        content.publishDate = content_;
                    }
                }
            }
            
            // 提取主要内容
            // 以下是一个简单的内容提取算法，实际应用中可能需要更复杂的算法
            
            // 1. 尝试找到常见的内容容器
            std::shared_ptr<HTMLNode> mainContent = nullptr;
            
            // 按可能包含主要内容的ID查找
            for (const auto& contentId : {"content", "main-content", "article", "post", "entry"}) {
                mainContent = findElementById(root, contentId);
                if (mainContent) break;
            }
            
            // 如果没有找到ID，则尝试查找语义化标签
            if (!mainContent) {
                auto articles = getElementsByTagName(root, "article");
                if (!articles.empty()) {
                    mainContent = articles[0];
                } else {
                    auto mains = getElementsByTagName(root, "main");
                    if (!mains.empty()) {
                        mainContent = mains[0];
                    } else {
                        // 如果都没找到，尝试使用body作为主要内容
                        auto bodies = getElementsByTagName(root, "body");
                        if (!bodies.empty()) {
                            mainContent = bodies[0];
                        }
                    }
                }
            }
            
            // 2. 提取主要内容文本
            if (mainContent) {
                content.content = getTextContent(mainContent);
            }
            
            // 3. 提取所有链接
            auto links = getElementsByTagName(root, "a");
            for (std::shared_ptr<HTMLNode> link : links) {
                auto href = link->attributes.find("href");
                if (href != link->attributes.end()) {
                    std::string anchorText = getTextContent(link);
                    content.links.push_back({href->second, anchorText});
                }
            }
            
        }
        return content;
    }

        // 清理HTML文本
    static std::string cleanText(const std::string& text) {
        std::string result;
        bool inSpace = false;
        
        for (char c : text) {
            if (isspace(c)) {
                if (!inSpace) {
                    result += ' ';
                    inSpace = true;
                }
            } else {
                result += c;
                inSpace = false;
            }
        }
        
        // 去除两端空格
        if (!result.empty()) {
            size_t start = result.find_first_not_of(' ');
            size_t end = result.find_last_not_of(' ');
            if (start != std::string::npos && end != std::string::npos) {
                result = result.substr(start, end - start + 1);
            } else {
                result.clear();
            }
        }
        
        return result;
    }
private:
    // 通过标签名查找元素
    void queryByTagName(std::shared_ptr<HTMLNode> node, const std::string& tagName, std::vector<std::shared_ptr<HTMLNode>>& result) {
        if (!node) {
            return;
        }
        
        if (node->type == NodeType::ELEMENT && node->name == tagName) {
            result.push_back(node);
        }
        
        for (std::shared_ptr<HTMLNode> child : node->children) {
            queryByTagName(child, tagName, result);
        }
    }
    
    // 通过ID查找元素
    std::shared_ptr<HTMLNode> findElementById(std::shared_ptr<HTMLNode> node, const std::string& id) {
        if (!node) {
            return nullptr;
        }
        
        if (node->type == NodeType::ELEMENT) {
            auto it = node->attributes.find("id");
            if (it != node->attributes.end() && it->second == id) {
                return node;
            }
        }
        
        for (std::shared_ptr<HTMLNode> child : node->children) {
            std::shared_ptr<HTMLNode> result = findElementById(child, id);
            if (result) {
                return result;
            }
        }
        
        return nullptr;
    }
};
