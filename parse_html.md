# 解析html网页内容
## html结构
```
<!DOCTYPE html>
<html>
  <head> <!-- 元数据和资源链接 -->
    <meta charset="UTF-8"> <!-- 必须声明字符编码[3,9](@ref) -->
    <title>页面标题</title>
  </head>
  <body>
    <header>页眉</header>
    <main> <!-- 主要内容区 -->
      <article> <!-- 独立内容区块 -->
        <h1>主标题</h1>
        <div class="content">...</div>
      </article>
    </main>
    <footer>页脚</footer>
  </body>
</html>
```
现代HTML网页通常包含以下主要部分：
1. DOCTYPE声明：指定HTML版本，如`<!DOCTYPE html>`表示HTML5
2. HTML元素：整个网页的根元素
3. HEAD部分：包含元数据、样式表、脚本链接等
4. BODY部分：包含实际显示的内容
5. 语义化标签：如`<header>, <nav>, <main>, <article>, <section>, <aside>, <footer>`等
6. 各类内容元素：如`<div>, <p>, <span>, <img>, <table>`等
7. 脚本和样式：`<script>`和`<style>`标签
8. 动态内容：由JavaScript生成的内容