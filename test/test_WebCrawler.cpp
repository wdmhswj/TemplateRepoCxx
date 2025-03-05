#include "WebCrawler.h"

int main(int argc, char** argv) {
    WebCrawler::Config config;
    // config.max_threads = 2;
    WebCrawler crawler("test.db", config);

    crawler.addUrl("https://yhirose.github.io");
    crawler.addUrl("http://example.com");
    crawler.addUrl("https://baidu.com");
    // crawler.addUrl("https://www.bilibili.com");
    crawler.start();

    std::this_thread::sleep_for(std::chrono::seconds(5));
    crawler.stop();

    return 0;
}