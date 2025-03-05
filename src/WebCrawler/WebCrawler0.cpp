#include "WebCrawler.h"

WebCrawler::WebCrawler(const Config& config) : config_(config), running_(false), active_threads_(0) {
    // 初始化统计信息
    stats_["total_urls"] = 0;
    stats_["successful_urls"] = 0;
    stats_["failed_urls"] = 0;
    stats_["skipped_urls"] = 0;
}

WebCrawler::~WebCrawler() {
    stop();
    waitForCompletion();
}

void WebCrawler::addUrl(const std::string& url, int depth) {
    std::lock_guard<std::mutex> lock(mutex_);
    enqueueUrl(url, depth);
}

void WebCrawler::enqueueUrl(const std::string& url, int depth) {
    // 规范化URL
    std::string normalized_url = normalizeUrl("", url);
    
    // 如果URL已经访问过或者超过最大深度，则跳过
    if (visited_urls_.find(normalized_url) != visited_urls_.end() || 
        depth > config_.max_depth || 
        visited_urls_.size() >= config_.max_urls) {
        stats_["skipped_urls"]++;
        return;
    }
    
    // 检查URL是否允许爬取
    if (config_.respect_robots_txt && !isUrlAllowed(normalized_url)) {
        stats_["skipped_urls"]++;
        return;
    }
    
    // 应用URL过滤器
    for (const auto& filter : url_filters_) {
        if (!filter(normalized_url)) {
            stats_["skipped_urls"]++;
            return;
        }
    }
    
    // 将URL添加到队列
    url_queue_.push(std::make_pair(normalized_url, depth));
    visited_urls_.insert(normalized_url);
    stats_["total_urls"]++;
    
    // 通知等待中的线程
    cv_.notify_one();
}

void WebCrawler::addUrlFilter(std::function<bool(const std::string&)> filter) {
    url_filters_.push_back(filter);
}

void WebCrawler::addContentHandler(std::function<void(const CrawlResult&)> handler) {
    content_handlers_.push_back(handler);
}

void WebCrawler::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        return;
    }
    
    running_ = true;
    
    // 创建工作线程
    for (int i = 0; i < config_.max_threads; i++) {
        worker_threads_.push_back(std::thread(&WebCrawler::workerThread, this));
    }
}

void WebCrawler::stop() {
    running_ = false;
    cv_.notify_all();
}

void WebCrawler::waitForCompletion() {
    // 等待所有线程完成
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
}

void WebCrawler::workerThread() {
    active_threads_++;
    
    while (running_) {
        std::pair<std::string, int> url_depth;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // 等待URL队列非空或者停止信号
            cv_.wait(lock, [this] {
                return !running_ || !url_queue_.empty();
            });
            
            // 如果停止信号且队列为空，则退出
            if (!running_ && url_queue_.empty()) {
                break;
            }
            
            // 获取URL和深度
            url_depth = url_queue_.front();
            url_queue_.pop();
        }
        
        // 爬取URL
        CrawlResult result = crawlUrl(url_depth.first, url_depth.second);
        
        // 如果爬取成功且内容类型是HTML，则提取链接
        if (result.success && result.content_type.find("text/html") != std::string::npos) {
            std::vector<std::string> links = extractLinks(result.body, result.url);
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                
                // 将提取的链接添加到队列
                for (const auto& link : links) {
                    enqueueUrl(link, result.depth + 1);
                }
            }
        }
        
        // 调用内容处理器
        for (const auto& handler : content_handlers_) {
            handler(result);
        }
        
        // 添加延迟以避免过快请求
        if (config_.delay_between_requests > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.delay_between_requests));
        }
    }
    
    active_threads_--;
}

WebCrawler::CrawlResult WebCrawler::crawlUrl(const std::string& url, int depth) {
    CrawlResult result;
    result.url = url;
    result.depth = depth;
    
    try {
        // 解析URL
        std::string scheme, host, path;
        int port = 0;
        
        std::regex url_regex("(https?)://([^:/]+)(?::(\\d+))?(/.*)");
        std::smatch matches;
        
        if (std::regex_match(url, matches, url_regex)) {
            scheme = matches[1].str();
            host = matches[2].str();
            port = matches[3].matched ? std::stoi(matches[3].str()) : (scheme == "https" ? 443 : 80);
            path = matches[4].str();
            if (path.empty()) {
                path = "/";
            }
        } else {
            result.success = false;
            result.error_message = "Invalid URL format";
            stats_["failed_urls"]++;
            return result;
        }
        
        // 创建HTTP客户端
        std::unique_ptr<httplib::Client> cli = createClient(url);
        if (!cli) {
            result.success = false;
            result.error_message = "Failed to create HTTP client";
            stats_["failed_urls"]++;
            return result;
        }
        
        // 设置超时
        cli->set_connection_timeout(config_.connection_timeout);
        cli->set_read_timeout(config_.read_timeout);
        
        // 设置代理
        if (!config_.proxy_host.empty() && config_.proxy_port > 0) {
            cli->set_proxy(config_.proxy_host.c_str(), config_.proxy_port);
            
            // 设置代理认证
            if (!config_.proxy_username.empty()) {
                cli->set_proxy_basic_auth(config_.proxy_username.c_str(), config_.proxy_password.c_str());
            }
        }
        
        // 设置请求头
        httplib::Headers headers = {
            {"User-Agent", config_.user_agent},
            {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8"},
            {"Accept-Language", "en-US,en;q=0.5"},
            {"Connection", "keep-alive"}
        };
        
        // 发送GET请求，最多重试config_.retry_count次
        httplib::Result res;
        int retry = 0;
        
        while (retry <= config_.retry_count) {
            res = cli->Get(path.c_str(), headers);
            
            if (res) {
                break;
            }
            
            retry++;
            
            if (retry <= config_.retry_count) {
                // 指数退避重试
                std::this_thread::sleep_for(std::chrono::milliseconds(500 * (1 << retry)));
            }
        }
        
        // 处理响应
        if (res) {
            result.status_code = res->status;
            
            // 处理重定向
            if ((result.status_code == 301 || result.status_code == 302) && config_.follow_redirects) {
                if (res->has_header("Location")) {
                    std::string redirect_url = res->get_header_value("Location");
                    redirect_url = normalizeUrl(url, redirect_url);
                    
                    // 递归调用以处理重定向
                    return crawlUrl(redirect_url, depth);
                }
            }
            
            // 填充结果
            result.content_type = res->has_header("Content-Type") ? res->get_header_value("Content-Type") : "";
            result.body = res->body;
            
            // 复制响应头
            for (const auto& header : res->headers) {
                result.headers[header.first] = header.second;
            }
            
            result.success = (result.status_code >= 200 && result.status_code < 300);
            
            if (result.success) {
                stats_["successful_urls"]++;
            } else {
                stats_["failed_urls"]++;
                result.error_message = "HTTP error: " + std::to_string(result.status_code);
            }
        } else {
            result.success = false;
            result.error_message = "HTTP request failed: " + httplib::to_string(res.error());
            stats_["failed_urls"]++;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Exception: " + std::string(e.what());
        stats_["failed_urls"]++;
    }
    
    return result;
}

std::string WebCrawler::normalizeUrl(const std::string& base_url, const std::string& url) {
    if (url.empty()) {
        return base_url;
    }
    
    // 如果是绝对URL，则直接返回
    if (url.find("http://") == 0 || url.find("https://") == 0) {
        return url;
    }
    
    // 解析基础URL
    std::string base_scheme, base_host, base_path;
    int base_port = 0;
    
    std::regex base_url_regex("(https?)://([^:/]+)(?::(\\d+))?(/.*)");
    std::smatch base_matches;
    
    if (std::regex_match(base_url, base_matches, base_url_regex)) {
        base_scheme = base_matches[1].str();
        base_host = base_matches[2].str();
        base_port = base_matches[3].matched ? std::stoi(base_matches[3].str()) : (base_scheme == "https" ? 443 : 80);
        base_path = base_matches[4].str();
        if (base_path.empty()) {
            base_path = "/";
        }
    } else {
        return url; // 无法解析基础URL，返回原始URL
    }
    
    if (url[0] == '/') {
        // 绝对路径
        if (url.size() > 1 && url[1] == '/') {
            // 协议相对URL
            return base_scheme + ":" + url;
        } else {
            // 主机相对URL
            std::string normalized = base_scheme + "://" + base_host;
            
            if (base_port != 80 && base_port != 443) {
                normalized += ":" + std::to_string(base_port);
            }
            
            normalized += url;
            return normalized;
        }
    } else {
        // 相对路径
        std::string dir_path = base_path.substr(0, base_path.find_last_of('/') + 1);
        std::string normalized = base_scheme + "://" + base_host;
        
        if (base_port != 80 && base_port != 443) {
            normalized += ":" + std::to_string(base_port);
        }
        
        normalized += dir_path + url;
        
        // 处理 "./" 和 "../"
        std::string result;
        std::vector<std::string> segments;
        std::istringstream path_stream(normalized.substr(normalized.find("://") + 3));
        std::string segment;
        
        std::getline(path_stream, segment, '/'); // 主机名
        result = normalized.substr(0, normalized.find("://") + 3) + segment + "/";
        
        while (std::getline(path_stream, segment, '/')) {
            if (segment == "..") {
                if (!segments.empty()) {
                    segments.pop_back();
                }
            } else if (segment != "." && !segment.empty()) {
                segments.push_back(segment);
            }
        }
        
        for (const auto& s : segments) {
            result += s + "/";
        }
        
        if (!segments.empty() && normalized.back() != '/') {
            result.pop_back();
        }
        
        return result;
    }
}

std::vector<std::string> WebCrawler::extractLinks(const std::string& html, const std::string& base_url) {
    std::vector<std::string> links;
    
    // 正则表达式匹配<a>标签中的href属性
    std::regex link_regex("<a[^>]*href=['\"]([^'\"]+)['\"][^>]*>");
    
    auto links_begin = std::sregex_iterator(html.begin(), html.end(), link_regex);
    auto links_end = std::sregex_iterator();
    
    for (std::sregex_iterator i = links_begin; i != links_end; ++i) {
        std::smatch match = *i;
        std::string href = match[1].str();
        
        // 忽略JavaScript链接和锚点
        if (href.find("javascript:") == 0 || href.find("#") == 0) {
            continue;
        }
        
        // 规范化URL
        std::string normalized_url = normalizeUrl(base_url, href);
        
        // 只添加HTTP和HTTPS链接
        if (normalized_url.find("http://") == 0 || normalized_url.find("https://") == 0) {
            links.push_back(normalized_url);
        }
    }
    
    return links;
}

void WebCrawler::parseRobotsTxt(const std::string& domain) {
    std::string url = "http://" + domain + "/robots.txt";
    
    try {
        // 创建HTTP客户端
        std::unique_ptr<httplib::Client> cli = createClient(url);
        if (!cli) {
            return;
        }
        
        // 设置超时
        cli->set_connection_timeout(config_.connection_timeout);
        cli->set_read_timeout(config_.read_timeout);
        
        // 设置请求头
        httplib::Headers headers = {
            {"User-Agent", config_.user_agent}
        };
        
        // 发送GET请求
        auto res = cli->Get("/robots.txt", headers);
        
        if (res && res->status == 200) {
            std::istringstream stream(res->body);
            std::string line;
            bool is_relevant_agent = false;
            
            // 解析robots.txt
            while (std::getline(stream, line)) {
                // 删除注释
                size_t comment_pos = line.find('#');
                if (comment_pos != std::string::npos) {
                    line = line.substr(0, comment_pos);
                }
                
                // 删除首尾空格
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);
                
                if (line.empty()) {
                    continue;
                }
                
                // 检查User-agent行
                if (line.find("User-agent:") == 0) {
                    std::string agent = line.substr(11);
                    agent.erase(0, agent.find_first_not_of(" \t"));
                    
                    // 检查是否与我们的User-agent相关
                    is_relevant_agent = (agent == "*" || config_.user_agent.find(agent) != std::string::npos);
                } 
                // 检查Disallow行
                else if (is_relevant_agent && line.find("Disallow:") == 0) {
                    std::string path = line.substr(9);
                    path.erase(0, path.find_first_not_of(" \t"));
                    
                    if (!path.empty()) {
                        robots_disallowed_[domain].insert(path);
                    }
                }
            }
        }
    } catch (const std::exception&) {
        // 忽略解析robots.txt时的异常
    }
}

bool WebCrawler::isUrlAllowed(const std::string& url) {
    std::string domain = getDomain(url);
    
    // 如果尚未解析该域名的robots.txt，则解析它
    if (robots_disallowed_.find(domain) == robots_disallowed_.end()) {
        parseRobotsTxt(domain);
    }
    
    // 检查URL路径是否被robots.txt禁止
    std::string path = url.substr(url.find(domain) + domain.length());
    if (path.empty()) {
        path = "/";
    }
    
    for (const auto& disallowed_path : robots_disallowed_[domain]) {
        if (disallowed_path == "/" || path.find(disallowed_path) == 0) {
            return false;
        }
    }
    
    return true;
}

std::string WebCrawler::getDomain(const std::string& url) {
    std::regex domain_regex("(https?)://([^:/]+)");
    std::smatch matches;
    
    if (std::regex_search(url, matches, domain_regex)) {
        return matches[2].str();
    }
    
    return "";
}

std::unique_ptr<httplib::Client> WebCrawler::createClient(const std::string& url) {
    std::cout << "createClient" << std::endl;
    std::string scheme, host;
    int port = 0;
    
    // std::regex url_regex("(https?)://([^:/]+)(?::(\\d+))?");
    std::regex url_regex("(https?)://([^:/]+)(?::(\\d+))?(?:/.*)?");
    std::smatch matches;
    
    if (std::regex_match(url, matches, url_regex)) {
        scheme = matches[1].str();
        host = matches[2].str();
        port = matches[3].matched ? std::stoi(matches[3].str()) : (scheme == "https" ? 443 : 80);
        
        if (scheme == "https") {
            #ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            return std::make_unique<httplib::SSLClient>(host.c_str(), port);
            // return std::unique_ptr<httplib::Client>(new httplib::SSLClient(host.c_str(), port));
            #else
            std::cout << "OpenSSL support not available" << std::endl;
            return nullptr; // 没有OpenSSL支持
            #endif
        } else {
            return std::make_unique<httplib::Client>(host.c_str(), port);
        }
    }
    std::cout << "not match" << std::endl;
    return nullptr;
}

void WebCrawler::saveUrlsToFile(const std::string& filename) {
    std::ofstream file(filename);
    
    if (file.is_open()) {
        for (const auto& url : visited_urls_) {
            file << url << std::endl;
        }
        
        file.close();
    }
}

void WebCrawler::loadUrlsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (!line.empty()) {
                addUrl(line);
            }
        }
        
        file.close();
    }
}

std::unordered_map<std::string, int> WebCrawler::getStats() {
    return stats_;
}