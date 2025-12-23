#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

// Структура для хранения записи истории сортировки
struct SortingRecord {
    int id;
    int user_id;
    std::string original_array;
    std::string sorted_array;
    std::string timestamp;
};

class Database {
private:
    SQLHENV henv;
    SQLHDBC hdbc;
    bool connected;

public:
    Database();
    ~Database();

    bool open();
    bool isConnected() const;
    void close();

    // Проверка подключения к базе данных
    bool testConnection();

    // Работа с пользователями
    bool addUser(const std::string& username, const std::string& password);
    int getUserId(const std::string& username);
    bool checkUser(const std::string& username, const std::string& password);

    // Работа с историей сортировки
    bool saveArray(const std::string& username,
        const std::vector<int>& original,
        const std::vector<int>& sorted);

    std::vector<std::string> getUserArrays(const std::string& username);
    std::vector<SortingRecord> getUserHistory(const std::string& username);

    // Вспомогательные функции
    void checkTableStructure();
    int getUserIDFromUsername(const std::string& username);
};