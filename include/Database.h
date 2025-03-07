#pragma once
#include <iostream>
#include <sqlite3.h>

class Database {
public:
    Database(const std::string& db_name) {
        if (sqlite3_open(db_name.c_str(), &db_) != SQLITE_OK) {
            std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        } else {
            db_opened = true;
            createTable();  // 🚀 自动创建表

        }
    }

    bool isOpened() const {
        return db_opened;
    }
    
    ~Database() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

    bool storePage(const std::string& url, int status_code, const std::string& content) {
        std::string sql = "INSERT INTO pages (url, status_code, content) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "SQL Error: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, status_code);
        sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    // ✅ 新增方法：读取数据库中所有存储的网页信息
    void fetchAllPages(int char_num = 50) {
        std::string sql = "SELECT id, url, status_code, content, fetch_time FROM pages;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "SQL Error: " << sqlite3_errmsg(db_) << std::endl;
            return;
        }

        std::cout << "📄 Stored Pages:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            std::string url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int status_code = sqlite3_column_int(stmt, 2);
            std::string content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            std::string fetch_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

            std::cout << "🆔 ID: " << id << "\n";
            std::cout << "🌐 URL: " << url << "\n";
            std::cout << "📡 Status: " << status_code << "\n";
            std::cout << "🕒 Time: " << fetch_time << "\n";
            std::cout << "📝 Content: " << content.substr(0, char_num) << "...\n"; // 仅显示前 50 个字符
            std::cout << "------------------------\n";
        }

        sqlite3_finalize(stmt);
    }

private:
    sqlite3* db_;
    bool db_opened = false;

    // 🚀 自动创建表
    void createTable() {
        std::string sql = "CREATE TABLE IF NOT EXISTS pages ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "url TEXT UNIQUE, "
                          "status_code INTEGER, "
                          "content TEXT, "
                          "fetch_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";
        char* errMsg = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
            std::cerr << "SQL Error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }
};
