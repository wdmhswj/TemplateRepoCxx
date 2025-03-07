#pragma once

#include <sqlite3.h>
#include <string>
#include "Structs.h"

// 数据库管理类
class DatabaseManager {
private:
    sqlite3* db;
    std::string lastError;
    
    // 执行SQL语句
    bool executeSQL(const std::string& sql) {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        
        if (rc != SQLITE_OK) {
            lastError = errMsg;
            sqlite3_free(errMsg);
            return false;
        }
        
        return true;
    }

    // 查询并打印表的内容
    void queryAndPrintTable(const std::string& sqlQuery) const {
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sqlQuery.c_str(), -1, &stmt, nullptr);

        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return;
        }

        int columnCount = sqlite3_column_count(stmt);

        // 打印列名
        for (int i = 0; i < columnCount; ++i) {
            std::cout << sqlite3_column_name(stmt, i) << "\t";
        }
        std::cout << std::endl;

        // 打印每行数据
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            for (int i = 0; i < columnCount; ++i) {
                const unsigned char* text = sqlite3_column_text(stmt, i);
                std::cout << (text ? reinterpret_cast<const char*>(text) : "NULL") << "\t";
            }
            std::cout << std::endl;
        }

        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to step through result set: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_finalize(stmt);
    }
    
public:
    DatabaseManager() : db(nullptr) {}
    
    ~DatabaseManager() {
        if (db) {
            sqlite3_close(db);
        }
    }
    
    // 打开或创建数据库
    bool open(const std::string& dbPath) {
        int rc = sqlite3_open(dbPath.c_str(), &db);
        
        if (rc != SQLITE_OK) {
            lastError = sqlite3_errmsg(db);
            sqlite3_close(db);
            db = nullptr;
            return false;
        }
        
        // 创建必要的表结构
        if (!createTables()) {
            return false;
        }
        
        return true;
    }
    
    // 创建表结构
    bool createTables() {
        // 创建网页内容表
        std::string sql = "CREATE TABLE IF NOT EXISTS web_contents ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "url TEXT UNIQUE NOT NULL,"
                          "title TEXT,"
                          "description TEXT,"
                          "author TEXT,"
                          "publish_date TEXT,"
                          "content TEXT,"
                          "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                          ");";
        
        if (!executeSQL(sql)) {
            return false;
        }
        
        // 创建关键词表
        sql = "CREATE TABLE IF NOT EXISTS keywords ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "content_id INTEGER,"
              "keyword TEXT,"
              "FOREIGN KEY (content_id) REFERENCES web_contents (id)"
              ");";
        
        if (!executeSQL(sql)) {
            return false;
        }
        
        // 创建链接表
        sql = "CREATE TABLE IF NOT EXISTS links ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "content_id INTEGER,"
              "url TEXT,"
              "anchor_text TEXT,"
              "FOREIGN KEY (content_id) REFERENCES web_contents (id)"
              ");";
        
        if (!executeSQL(sql)) {
            return false;
        }
        
        // 创建元数据表
        sql = "CREATE TABLE IF NOT EXISTS metadata ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "content_id INTEGER,"
              "name TEXT,"
              "value TEXT,"
              "FOREIGN KEY (content_id) REFERENCES web_contents (id)"
              ");";
        
        return executeSQL(sql);
    }
    
    // 保存网页内容到数据库
    bool saveContent(const WebContent& content) {
        // 准备SQL语句
        sqlite3_stmt* stmt;
        std::string sql = "INSERT OR REPLACE INTO web_contents (url, title, description, author, publish_date, content) "
                          "VALUES (?, ?, ?, ?, ?, ?);";
                          
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            lastError = sqlite3_errmsg(db);
            return false;
        }
        
        // 绑定参数
        sqlite3_bind_text(stmt, 1, content.url.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, content.title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, content.description.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, content.author.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, content.publishDate.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, content.content.c_str(), -1, SQLITE_STATIC);
        
        // 执行插入
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            lastError = sqlite3_errmsg(db);
            return false;
        }
        
        // 获取插入的内容ID
        int64_t contentId = sqlite3_last_insert_rowid(db);
        
        // 保存关键词
        for (const auto& keyword : content.keywords) {
            sql = "INSERT INTO keywords (content_id, keyword) VALUES (?, ?);";
            rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
            
            if (rc != SQLITE_OK) {
                lastError = sqlite3_errmsg(db);
                continue;
            }
            
            sqlite3_bind_int64(stmt, 1, contentId);
            sqlite3_bind_text(stmt, 2, keyword.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        // 保存链接
        for (const auto& link : content.links) {
            sql = "INSERT INTO links (content_id, url, anchor_text) VALUES (?, ?, ?);";
            rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
            
            if (rc != SQLITE_OK) {
                lastError = sqlite3_errmsg(db);
                continue;
            }
            
            sqlite3_bind_int64(stmt, 1, contentId);
            sqlite3_bind_text(stmt, 2, link.first.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, link.second.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        // 保存元数据
        for (const auto& meta : content.metadata) {
            sql = "INSERT INTO metadata (content_id, name, value) VALUES (?, ?, ?);";
            rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
            
            if (rc != SQLITE_OK) {
                lastError = sqlite3_errmsg(db);
                continue;
            }
            
            sqlite3_bind_int64(stmt, 1, contentId);
            sqlite3_bind_text(stmt, 2, meta.first.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, meta.second.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        return true;
    }
    
    // 获取最近的错误信息
    std::string getLastError() const {
        return lastError;
    }
    
    // 打印数据库内容
    void printDatabaseContents() const {
        // 查询并打印web_contents表
        std::cout << "Web Contents:\n";
        queryAndPrintTable("SELECT * FROM web_contents");

        // 查询并打印keywords表
        std::cout << "\nKeywords:\n";
        queryAndPrintTable("SELECT k.id, wc.url, k.keyword FROM keywords AS k JOIN web_contents AS wc ON k.content_id = wc.id");

        // 查询并打印links表
        std::cout << "\nLinks:\n";
        queryAndPrintTable("SELECT l.id, wc.url, l.url AS link_url, l.anchor_text FROM links AS l JOIN web_contents AS wc ON l.content_id = wc.id");

        // 查询并打印metadata表
        std::cout << "\nMetadata:\n";
        queryAndPrintTable("SELECT m.id, wc.url, m.name, m.value FROM metadata AS m JOIN web_contents AS wc ON m.content_id = wc.id");
    }
};