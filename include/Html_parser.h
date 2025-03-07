#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <fstream>

// 定义HTML节点类型
enum class NodeType {
    ELEMENT,
    TEXT,
    COMMENT,
    DOCTYPE
};

// HTML节点结构
struct HTMLNode {
    NodeType type;
    std::string name;
    std::string content;
    std::map<std::string, std::string> attributes;
    HTMLNode* parent;
    std::vector<HTMLNode*> children;
    
    HTMLNode(NodeType t) : type(t), parent(nullptr) {}
    ~HTMLNode() {
        for (auto child : children) {
            delete child;
        }
    }
};

// HTML解析器类
class HTMLParser {
private:
    std::string html;
    size_t pos = 0;
    HTMLNode* root;
    
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
    HTMLNode* parseComment() {
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
        
        HTMLNode* node = new HTMLNode(NodeType::COMMENT);
        node->content = content;
        return node;
    }
    
    // 解析DOCTYPE
    HTMLNode* parseDoctype() {
        // 跳过 <!DOCTYPE
        size_t start = pos;
        while (pos < html.length() && html[pos] != '>') {
            pos++;
        }
        if (pos < html.length()) {
            pos++; // 跳过 >
        }
        
        HTMLNode* node = new HTMLNode(NodeType::DOCTYPE);
        node->content = html.substr(start, pos - start);
        return node;
    }
    
    // 解析元素
    HTMLNode* parseElement() {
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
        HTMLNode* node = new HTMLNode(NodeType::ELEMENT);
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
    HTMLNode* parseText() {
        std::string content;
        while (pos < html.length() && html[pos] != '<') {
            content += html[pos++];
        }
        
        // 忽略纯空白的文本节点
        if (std::all_of(content.begin(), content.end(), isspace)) {
            return nullptr;
        }
        
        HTMLNode* node = new HTMLNode(NodeType::TEXT);
        node->content = content;
        return node;
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
    void parseNodes(HTMLNode* parent) {
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
            HTMLNode* node = parseNode();
            if (node) {
                node->parent = parent;
                parent->children.push_back(node);
            }
        }
    }
    
    // 解析节点
    HTMLNode* parseNode() {
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
    
    ~HTMLParser() {
        if (root) {
            delete root;
        }
    }
    
    // 解析HTML字符串
    HTMLNode* parse(const std::string& htmlContent) {
        html = htmlContent;
        pos = 0;
        
        // 创建根节点
        root = new HTMLNode(NodeType::ELEMENT);
        root->name = "document";
        
        // 解析所有子节点
        while (pos < html.length()) {
            HTMLNode* node = parseNode();
            if (node) {
                node->parent = root;
                root->children.push_back(node);
            }
        }
        
        return root;
    }
    
    // 从文件中解析HTML
    HTMLNode* parseFromFile(const std::string& filename) {
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
    void printTree(HTMLNode* node, int depth = 0) {
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
        
        for (HTMLNode* child : node->children) {
            printTree(child, depth + 1);
        }
        
        if (node->type == NodeType::ELEMENT) {
            std::cout << indent << "</" << node->name << ">" << std::endl;
        }
    }
    
    // 使用CSS选择器查找元素（简化版本）
    std::vector<HTMLNode*> querySelector(const std::string& selector) {
        std::vector<HTMLNode*> result;
        if (!root) {
            return result;
        }
        
        // 简单实现：仅支持标签名选择器
        queryByTagName(root, selector, result);
        return result;
    }
    
    // 通过ID查找元素
    HTMLNode* getElementById(const std::string& id) {
        if (!root) {
            return nullptr;
        }
        
        return findElementById(root, id);
    }
    
private:
    // 通过标签名查找元素
    void queryByTagName(HTMLNode* node, const std::string& tagName, std::vector<HTMLNode*>& result) {
        if (!node) {
            return;
        }
        
        if (node->type == NodeType::ELEMENT && node->name == tagName) {
            result.push_back(node);
        }
        
        for (HTMLNode* child : node->children) {
            queryByTagName(child, tagName, result);
        }
    }
    
    // 通过ID查找元素
    HTMLNode* findElementById(HTMLNode* node, const std::string& id) {
        if (!node) {
            return nullptr;
        }
        
        if (node->type == NodeType::ELEMENT) {
            auto it = node->attributes.find("id");
            if (it != node->attributes.end() && it->second == id) {
                return node;
            }
        }
        
        for (HTMLNode* child : node->children) {
            HTMLNode* result = findElementById(child, id);
            if (result) {
                return result;
            }
        }
        
        return nullptr;
    }
};
