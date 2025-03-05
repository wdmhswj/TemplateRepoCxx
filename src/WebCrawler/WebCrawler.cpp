#include "WebCrawler.h"
#include "UrlParser.h"

WebCrawler::WebCrawler(const std::string& db_name, const Config& config) 
: m_config(config)
, m_running(false)
, m_db(db_name){
    if(m_db.isOpened())
        m_db_opened = true;
}

WebCrawler::~WebCrawler() {
    stop();
}
void WebCrawler::addUrl(const std::string& url, int depth) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_visited_urls.find(url) == m_visited_urls.end())
    {
        m_url_queue.emplace(url, depth);
        m_visited_urls.insert(url);
        m_cv.notify_one();
    }   
}
void WebCrawler::start() {
    m_running = true;
    for (int i = 0; i < m_config.max_depth; ++i)
    {
        m_workers.emplace_back(&WebCrawler::workerThread, this);
    }   
}
void WebCrawler::stop() {
    m_running = false;
    m_cv.notify_all();
    for(auto& worker: m_workers) {
        if(worker.joinable()) {
            worker.join();
        }
    }
}
void WebCrawler::workerThread() {
    while (m_running)
    {
        std::string url;
        int depth;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return !m_url_queue.empty() || !m_running; });

            if(!m_running || m_url_queue.empty())
                return;
            std::tie(url, depth) = m_url_queue.front();
            m_url_queue.pop();

        }
        CrawlResult result = crawUrl(url);
        if(result.success) {
            std::cout << "[SUCCESS] " << url << " (" << result.status_code << ")\n";
            // std::cout << result.body << std::endl;
            if(m_db_opened) {
                if(m_db.storePage(url, result.status_code, result.body)) {
                    std::cout << "✅ Page stored successfully!\n";
                } else {
                    std::cout << "❌ Failed to store page.\n";
                }
            }

        } else {
            std::cout << "[FAILED] " << url << " (" << result.status_code << ") " << result.error_message << std::endl;
            // std::cout << result.body << std::endl;
        }
    }
}
WebCrawler::CrawlResult WebCrawler::crawUrl(const std::string& url) {
    CrawlResult result{url, 0, "", false, ""};
    // 解析url
    auto parsed_url = UrlParser::parse(url);
    std::string proto;
    int port = 0;
    std::string path;
    std::string host;
    if(parsed_url) {
        proto = parsed_url->protocol;
        port = parsed_url->port;
        path = parsed_url->path;
        host = parsed_url->domain;
    } else {
        result.error_message = "URL 解析失败";
        return result;    
    }
    // 判断 http or https
    if(proto=="http") {

        httplib::Client client(host, port);
        client.set_ca_cert_path("./ca-bundle.crt");
        // Disable cert verification
        client.enable_server_certificate_verification(false);

        // Disable host verification
        client.enable_server_hostname_verification(false);


        // 配置客户端
        client.set_follow_location(m_config.follow_redirects);
        client.set_connection_timeout(1); 
        client.set_read_timeout(5, 0); // 5秒

        // 准备请求头
        httplib::Headers headers;
        headers.insert({"User-Agent", m_config.user_agent});
        headers.insert({"Accept", "*/*"});
        headers.insert({"Connection", "close"});

        try {
            
            auto res = client.Get("/", headers);
            
            if (!res) {
                auto err = res.error();
                result.error_message = httplib::to_string(err);
                return result;
            }

            // 检查成功的状态码
            result.status_code = res->status;
            result.body = res->body;
            result.success = (res->status >= 200 && res->status < 300);

            return result;
        }
        catch (const std::exception& e) {
            result.error_message = e.what();
            return result;
        }

    }else if(proto=="https") {
        httplib::SSLClient client(host, port);

        // 配置客户端
        client.set_follow_location(m_config.follow_redirects);
        client.set_connection_timeout(1); // 300毫秒
        client.set_read_timeout(5, 0); // 5秒

        // 准备请求头
        httplib::Headers headers;
        headers.insert({"User-Agent", m_config.user_agent});
        headers.insert({"Accept", "*/*"});
        headers.insert({"Connection", "close"});


        try {
            
            auto res = client.Get("/", headers);
            
            if (!res) {
                auto err = res.error();
                result.error_message = httplib::to_string(err);
                return result;
            }

            // 检查成功的状态码
            result.status_code = res->status;
            result.body = res->body;
            result.success = (res->status >= 200 && res->status < 300);

            return result;
        }
        catch (const std::exception& e) {
            result.error_message = e.what();
            return result;
        }
    }

    result.error_message = "在暂时只支持http/https协议";
    return result;
}