// WebCrawler.h
#ifndef WEB_CRAWLER_H
#define WEB_CRAWLER_H
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <string>
#include <vector>
#include <queue>
#include <unordered_set>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <sstream>
#include <httplib/httplib.h>
#include <regex>
#include <chrono>
#include "Database.h"

class WebCrawler {
public:
    // 配置结构体
    struct Config {
        int max_threads = 4;    // 最大线程数
        int max_depth = 2;      // 最大爬取深度
        int max_urls = 100;     // 最大爬取url数目
        bool follow_redirects = true;  // 是否跟随HTTP重定向
        std::string user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"; // HTTP请求头的用户代理字段
        Config(int m_t, int m_d, int m_u, bool f_r)
        : max_threads(m_t)
        , max_depth(m_d)
        , max_urls(m_u)
        , follow_redirects(f_r) {}
        Config() {}
    };

    // 爬取结果结构体
    struct CrawlResult {
        std::string url;
        int status_code;    // 状态码
        std::string body;   // 爬取的网页内容主体
        bool success;       // 是否爬取成功
        std::string error_message;  // 错误信息
        CrawlResult(const std::string& u, int s_c, const std::string& b, bool s, const std::string& e_m)
        : url(u)
        , status_code(s_c)
        , body(b)
        , success(s)
        , error_message(e_m) {}
        CrawlResult() {}
    };

    // 构造/析构
    WebCrawler(const std::string& db_name, const Config& config = Config{});
    ~WebCrawler();

    // 成员函数
    void addUrl(const std::string& url, int depth = 0);
    void start();
    void stop();
    
private:
    // 成员变量
    Config m_config;
    std::queue<std::pair<std::string, int>> m_url_queue;    // 待爬取的URL及其和深度
    std::unordered_set<std::string> m_visited_urls; // 已爬取的URL的无序集合
    std::vector<std::thread> m_workers; // 爬取所用的线程对象
    std::mutex m_mutex; // 线程锁
    std::condition_variable m_cv;   // 条件变量，用于线程同步
    std::atomic<bool> m_running;    // 原子布尔值，确保多线程安全
    Database m_db;  // 网页内容存储数据库
    bool m_db_opened = false;   // 数据库是否正常打开

    // 成员函数
    void workerThread();
    CrawlResult crawUrl(const std::string& url);

};

#endif // WEB_CRAWLER_H