#include "database.h"
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <ctime>
#include <string>

#pragma comment(lib, "odbc32.lib")

using namespace std;

// ================= ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =================

wstring utf8_to_wstring(const string& str) {
    if (str.empty()) return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    if (size_needed <= 0) return L"";

    wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

string wstring_to_utf8(const wstring& wstr) {
    if (wstr.empty()) return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";

    string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

// ================= РЕАЛИЗАЦИЯ КЛАССА DATABASE =================

Database::Database() : henv(SQL_NULL_HENV), hdbc(SQL_NULL_HDBC), connected(false) {
}

Database::~Database() {
    close();
}

bool Database::open() {
    SQLRETURN retcode;

    // 1. Allocate environment handle
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        cerr << "Ошибка выделения окружения ODBC" << endl;
        return false;
    }

    // 2. Set the ODBC version
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        cerr << "Ошибка установки версии ODBC" << endl;
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // 3. Allocate connection handle
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        cerr << "Ошибка выделения соединения ODBC" << endl;
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // 4. Connect to SQL Server
    SQLWCHAR* dsn = (SQLWCHAR*)L"Driver={ODBC Driver 17 for SQL Server};"
        L"Server=localhost;"
        L"Database=SortingDB;"
        L"Trusted_Connection=yes;"
        L"Encrypt=no;";

    SQLWCHAR outstr[1024];
    SQLSMALLINT outstrlen;

    retcode = SQLDriverConnectW(hdbc, NULL, dsn, SQL_NTS,
        outstr, sizeof(outstr) / sizeof(outstr[0]),
        &outstrlen, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        connected = true;
        return true;
    }
    else {
        cerr << "Ошибка подключения к SQL Server" << endl;
        return false;
    }
}

bool Database::isConnected() const {
    return connected;
}

void Database::close() {
    if (connected) {
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        connected = false;
        henv = SQL_NULL_HENV;
        hdbc = SQL_NULL_HDBC;
    }
}

bool Database::addUser(const std::string& username, const std::string& password) {
    if (!connected) {
        cerr << "addUser: Нет подключения к БД" << endl;
        return false;
    }

    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode;

    // Проверяем, существует ли пользователь
    wstring checkSql = L"SELECT COUNT(*) FROM Users WHERE username = ?";

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode != SQL_SUCCESS) {
        cerr << "addUser: Ошибка выделения statement" << endl;
        return false;
    }

    // Подготавливаем запрос
    retcode = SQLPrepareW(hstmt, (SQLWCHAR*)checkSql.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        cerr << "addUser: Ошибка подготовки проверки" << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return false;
    }

    wstring wUsername = utf8_to_wstring(username);
    SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        100, 0, (SQLPOINTER)wUsername.c_str(), 0, NULL);

    // Выполняем проверку
    retcode = SQLExecute(hstmt);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        SQLINTEGER count = 0;
        SQLLEN count_ind;
        SQLFetch(hstmt);
        SQLGetData(hstmt, 1, SQL_C_SLONG, &count, 0, &count_ind);

        if (count > 0) {
            cerr << "addUser: Пользователь уже существует" << endl;
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            return false;
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    // Вставляем нового пользователя
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode != SQL_SUCCESS) {
        cerr << "addUser: Ошибка выделения statement для вставки" << endl;
        return false;
    }

    wstring insertSql = L"INSERT INTO Users (username, password_hash) VALUES (?, ?)";
    retcode = SQLPrepareW(hstmt, (SQLWCHAR*)insertSql.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        cerr << "addUser: Ошибка подготовки вставки" << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return false;
    }

    wstring wPassword = utf8_to_wstring(password);

    // Привязываем параметры
    SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        100, 0, (SQLPOINTER)wUsername.c_str(), 0, NULL);

    SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        255, 0, (SQLPOINTER)wPassword.c_str(), 0, NULL);

    // Выполняем вставку
    retcode = SQLExecute(hstmt);
    bool success = (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);

    if (!success) {
        cerr << "addUser: Ошибка выполнения вставки: " << retcode << endl;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    return success;
}

int Database::getUserId(const std::string& username) {
    if (!connected) return -1;

    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    if (retcode != SQL_SUCCESS) return -1;

    wstring sql = L"SELECT id FROM Users WHERE username = ?";
    retcode = SQLPrepareW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return -1;
    }

    wstring wUsername = utf8_to_wstring(username);
    SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        100, 0, (SQLPOINTER)wUsername.c_str(), 0, NULL);

    retcode = SQLExecute(hstmt);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLFetch(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            SQLINTEGER id;
            SQLLEN id_ind;
            SQLGetData(hstmt, 1, SQL_C_SLONG, &id, 0, &id_ind);

            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            return static_cast<int>(id);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    return -1;
}

bool Database::checkUser(const std::string& username, const std::string& password) {
    if (!connected) return false;

    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    if (retcode != SQL_SUCCESS) return false;

    wstring sql = L"SELECT COUNT(*) FROM Users WHERE username = ? AND password_hash = ?";
    retcode = SQLPrepareW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return false;
    }

    wstring wUsername = utf8_to_wstring(username);
    wstring wPassword = utf8_to_wstring(password);

    SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        100, 0, (SQLPOINTER)wUsername.c_str(), 0, NULL);

    SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        255, 0, (SQLPOINTER)wPassword.c_str(), 0, NULL);

    retcode = SQLExecute(hstmt);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLFetch(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            SQLINTEGER count;
            SQLLEN count_ind;
            SQLGetData(hstmt, 1, SQL_C_SLONG, &count, 0, &count_ind);

            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            return (count > 0);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    return false;
}

bool Database::saveArray(const std::string& username,
    const std::vector<int>& original,
    const std::vector<int>& sorted) {
    if (!connected) return false;

    int userId = getUserId(username);
    if (userId == -1) return false;

    // Преобразуем массивы в строки
    ostringstream origStream, sortedStream;
    for (size_t i = 0; i < original.size(); ++i) {
        if (i > 0) origStream << ",";
        origStream << original[i];
    }
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i > 0) sortedStream << ",";
        sortedStream << sorted[i];
    }

    string originalStr = origStream.str();
    string sortedStr = sortedStream.str();

    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    if (retcode != SQL_SUCCESS) return false;

    wstring sql = L"INSERT INTO SortingHistory (user_id, original_array, sorted_array) VALUES (?, ?, ?)";
    retcode = SQLPrepareW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return false;
    }

    SQLINTEGER userIDParam = static_cast<SQLINTEGER>(userId);
    wstring wOriginalStr = utf8_to_wstring(originalStr);
    wstring wSortedStr = utf8_to_wstring(sortedStr);

    SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &userIDParam, 0, NULL);
    SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        0, 0, (SQLPOINTER)wOriginalStr.c_str(), 0, NULL);
    SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        0, 0, (SQLPOINTER)wSortedStr.c_str(), 0, NULL);

    retcode = SQLExecute(hstmt);
    bool success = (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    return success;
}

std::vector<std::string> Database::getUserArrays(const std::string& username) {
    std::vector<std::string> result;
    if (!connected) return result;

    int userId = getUserId(username);
    if (userId == -1) return result;

    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    if (retcode != SQL_SUCCESS) return result;

    wstring sql = L"SELECT original_array, sorted_array, CONVERT(VARCHAR, timestamp, 120) FROM SortingHistory WHERE user_id = ? ORDER BY timestamp DESC";
    retcode = SQLPrepareW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return result;
    }

    SQLINTEGER userIDParam = static_cast<SQLINTEGER>(userId);
    SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &userIDParam, 0, NULL);

    retcode = SQLExecute(hstmt);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        while (SQLFetch(hstmt) == SQL_SUCCESS) {
            SQLWCHAR originalArray[1024] = { 0 };
            SQLWCHAR sortedArray[1024] = { 0 };
            SQLWCHAR timestamp[50] = { 0 };
            SQLLEN orig_ind, sorted_ind, time_ind;

            SQLGetData(hstmt, 1, SQL_C_WCHAR, originalArray, sizeof(originalArray) / sizeof(SQLWCHAR), &orig_ind);
            SQLGetData(hstmt, 2, SQL_C_WCHAR, sortedArray, sizeof(sortedArray) / sizeof(SQLWCHAR), &sorted_ind);
            SQLGetData(hstmt, 3, SQL_C_WCHAR, timestamp, sizeof(timestamp) / sizeof(SQLWCHAR), &time_ind);

            string entry = "Исходный: " + wstring_to_utf8(originalArray) +
                " -> Отсортированный: " + wstring_to_utf8(sortedArray) +
                " (" + wstring_to_utf8(timestamp) + ")";
            result.push_back(entry);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    return result;
}

// Остальные функции (заглушки)
bool Database::testConnection() {
    return connected;
}

std::vector<SortingRecord> Database::getUserHistory(const std::string& username) {
    return std::vector<SortingRecord>();
}

void Database::checkTableStructure() {
    // Пустая реализация
}

int Database::getUserIDFromUsername(const std::string& username) {
    return getUserId(username);
}