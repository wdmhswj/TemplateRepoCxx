#include "WebCrawler.h"
#include <iostream>

// 定义内容处理函数
void handleContent(const WebCrawler::CrawlResult& result) {
    if (result.success) {
        std::cout << "成功爬取: " << result.url << std::endl;
        std::cout << "内容类型: " << result.content_type << std::endl;
        std::cout << "内容长度: " << result.body.length() << " 字节" << std::endl;
        
        // 保存内容到文件
        std::string filename = "page_" + std::to_string(std::hash<std::string>{}(result.url)) + ".html";
        std::ofstream outfile(filename);
        if (outfile.is_open()) {
            outfile << result.body;
            outfile.close();
            std::cout << "内容已保存到 " << filename << std::endl;
        }
    } else {
        std::cout << "爬取失败: " << result.url << std::endl;
        std::cout << "错误信息: " << result.error_message << std::endl;
    }
}

int main() {
    // 创建配置
    WebCrawler::Config config;
    config.max_threads = 4;
    config.max_depth = 2;
    config.delay_between_requests = 1000; // 1秒
    config.user_agent = "MyCrawler/1.0 (https://baidu.com/bot)";
    
    // 创建爬虫实例
    WebCrawler crawler(config);
    
    // 添加URL过滤器（只爬取特定域名）
    crawler.addUrlFilter([](const std::string& url) {
        return url.find("baidu.com") != std::string::npos;
    });
    
    // 添加内容处理器
    crawler.addContentHandler(handleContent);
    
    // 添加初始URL
    crawler.addUrl("https://baidu.com/");
    
    // 开始爬取
    crawler.start();
    
    // 等待完成
    crawler.waitForCompletion();
    
    // 打印统计信息
    auto stats = crawler.getStats();
    std::cout << "总URL数: " << stats["total_urls"] << std::endl;
    std::cout << "成功URL数: " << stats["successful_urls"] << std::endl;
    std::cout << "失败URL数: " << stats["failed_urls"] << std::endl;
    std::cout << "跳过URL数: " << stats["skipped_urls"] << std::endl;
    
    return 0;
}