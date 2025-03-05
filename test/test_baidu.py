import requests

def crawl_baidu():
    try:
        # 设置User-Agent，模拟浏览器访问
        headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
        }
        
        # 发送GET请求
        response = requests.get('https://www.baidu.com', headers=headers, timeout=10)
        
        # 检查请求是否成功
        if response.status_code == 200:
            # 返回网页源代码
            return response.text
        else:
            print(f"请求失败，状态码：{response.status_code}")
            return None
    
    except requests.RequestException as e:
        print(f"爬取过程中发生错误：{e}")
        return None

def save_content(content):
    if content:
        try:
            with open('baidu_homepage.html', 'w', encoding='utf-8') as f:
                f.write(content)
            print("百度首页内容已成功保存到 baidu_homepage.html")
        except IOError as e:
            print(f"保存文件时发生错误：{e}")

def main():
    # 爬取百度首页
    baidu_content = crawl_baidu()
    
    # 保存内容
    save_content(baidu_content)

if __name__ == '__main__':
    main()