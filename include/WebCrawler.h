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

class WebCrawler {
public:
    // 爬虫配置结构体
    // struct Config {
    //     int max_threads = 4;                  // 最大线程数
    //     int connection_timeout = 5;           // 连接超时（秒）
    //     int read_timeout = 5;                 // 读取超时（秒）
    //     int retry_count = 3;                  // 重试次数
    //     int delay_between_requests = 500;     // 请求间隔（毫秒）
    //     int max_depth = 2;                    // 最大爬取深度
    //     int max_urls = 100;                   // 最大URL数量
    //     bool follow_redirects = true;         // 是否跟随重定向
    //     bool respect_robots_txt = true;       // 是否尊重robots.txt
    //     std::string user_agent = "Mozilla/5.0 (compatible; MyCrawler/1.0)";
    //     std::string proxy_host = "";          // 代理主机
    //     int proxy_port = 0;                   // 代理端口
    //     std::string proxy_username = "";      // 代理用户名
    //     std::string proxy_password = "";      // 代理密码
    // };
struct Config {
        int max_threads;                  // 最大线程数
        int connection_timeout;           // 连接超时（秒）
        int read_timeout;                 // 读取超时（秒）
        int retry_count;                  // 重试次数
        int delay_between_requests;       // 请求间隔（毫秒）
        int max_depth;                    // 最大爬取深度
        int max_urls;                     // 最大URL数量
        bool follow_redirects;            // 是否跟随重定向
        bool respect_robots_txt;          // 是否尊重robots.txt
        std::string user_agent;
        std::string proxy_host;           // 代理主机
        int proxy_port;                   // 代理端口
        std::string proxy_username;       // 代理用户名
        std::string proxy_password;       // 代理密码
        
        // 构造函数，设置默认值
        Config() : 
            max_threads(4),
            connection_timeout(5),
            read_timeout(5),
            retry_count(3),
            delay_between_requests(500),
            max_depth(2),
            max_urls(100),
            follow_redirects(true),
            respect_robots_txt(true),
            user_agent("Mozilla/5.0 (compatible; MyCrawler/1.0)"),
            proxy_host(""),
            proxy_port(0),
            proxy_username(""),
            proxy_password("")
        {}
    };
    // 爬取结果结构体
    struct CrawlResult {
        std::string url;                      // 爬取的URL
        int status_code = 0;                  // HTTP状态码
        std::string content_type;             // 内容类型
        std::string body;                     // 响应体
        std::unordered_map<std::string, std::string> headers; // 响应头
        bool success = false;                 // 是否成功
        std::string error_message;            // 错误信息
        int depth = 0;                        // 爬取深度
    };

    // 构造函数
    WebCrawler(const Config& config = Config());
    
    // 析构函数
    ~WebCrawler();

    // 添加初始URL
    void addUrl(const std::string& url, int depth = 0);
    
    // 添加URL过滤器（返回true表示接受该URL）
    void addUrlFilter(std::function<bool(const std::string&)> filter);
    
    // 添加内容处理器
    void addContentHandler(std::function<void(const CrawlResult&)> handler);
    
    // 开始爬取
    void start();
    
    // 停止爬取
    void stop();
    
    // 等待爬取完成
    void waitForCompletion();
    
    // 保存URL到文件
    void saveUrlsToFile(const std::string& filename);
    
    // 从文件加载URL
    void loadUrlsFromFile(const std::string& filename);
    
    // 获取爬取结果统计
    std::unordered_map<std::string, int> getStats();

private:
    // 规范化URL
    std::string normalizeUrl(const std::string& base_url, const std::string& url);
    
    // 从HTML中提取链接
    std::vector<std::string> extractLinks(const std::string& html, const std::string& base_url);
    
    // 解析robots.txt
    void parseRobotsTxt(const std::string& domain);
    
    // 检查URL是否允许爬取
    bool isUrlAllowed(const std::string& url);
    
    // 爬取单个URL
    CrawlResult crawlUrl(const std::string& url, int depth);
    
    // 工作线程函数
    void workerThread();
    
    // 添加URL到队列（内部使用）
    void enqueueUrl(const std::string& url, int depth);
    
    // 获取域名
    std::string getDomain(const std::string& url);
    
    // 创建HTTP客户端
    std::unique_ptr<httplib::Client> createClient(const std::string& url);

    // 成员变量
    Config config_;
    std::queue<std::pair<std::string, int>> url_queue_;     // URL队列 <url, depth>
    std::unordered_set<std::string> visited_urls_;          // 已访问URL集合
    std::unordered_map<std::string, std::unordered_set<std::string>> robots_disallowed_; // robots.txt禁止的路径
    std::vector<std::function<bool(const std::string&)>> url_filters_; // URL过滤器
    std::vector<std::function<void(const CrawlResult&)>> content_handlers_; // 内容处理器
    std::vector<std::thread> worker_threads_;               // 工作线程
    std::mutex mutex_;                                      // 互斥锁
    std::condition_variable cv_;                            // 条件变量
    std::atomic<bool> running_;                             // 是否运行中
    std::atomic<int> active_threads_;                       // 活动线程数
    std::unordered_map<std::string, int> stats_;            // 统计信息
};

#endif // WEB_CRAWLER_H