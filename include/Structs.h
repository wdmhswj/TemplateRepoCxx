#pragma once
#include <string>
#include <vector>
#include <map>

// 定义HTML节点类型
enum class NodeType {
    ELEMENT,
    TEXT,
    COMMENT,
    DOCTYPE
};


// 定义数据库记录结构
struct WebContent {
    int id;
    std::string url;
    std::string title;
    std::string description;
    std::string author;
    std::string publishDate;
    std::string content;
    std::vector<std::string> keywords;
    std::vector<std::pair<std::string, std::string>> links; // URL和锚文本
    std::map<std::string, std::string> metadata;
};