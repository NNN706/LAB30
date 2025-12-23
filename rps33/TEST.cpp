#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>
#include <string>

#pragma comment(lib, "odbc32.lib")

using namespace std;

int main() {
    system("chcp 1251 > nul");

    cout << "=== ПРОСТОЙ ТЕСТ ODBC ПОДКЛЮЧЕНИЯ ===" << endl;

    SQLHENV henv = SQL_NULL_HENV;
    SQLHDBC hdbc = SQL_NULL_HDBC;
    SQLRETURN retcode;

    // 1. Allocate environment
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (retcode != SQL_SUCCESS) {
        cout << "ОШИБКА: Не удалось выделить окружение ODBC" << endl;
        return 1;
    }
    cout << "1. Окружение ODBC создано" << endl;

    // 2. Set ODBC version
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS) {
        cout << "ОШИБКА: Не удалось установить версию ODBC" << endl;
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return 1;
    }
    cout << "2. Версия ODBC 3.0 установлена" << endl;

    // 3. Allocate connection
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (retcode != SQL_SUCCESS) {
        cout << "ОШИБКА: Не удалось выделить соединение" << endl;
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return 1;
    }
    cout << "3. Соединение выделено" << endl;

    // 4. Connect to SQL Server
    SQLWCHAR connStr[] = L"DRIVER={ODBC Driver 17 for SQL Server};SERVER=localhost;DATABASE=SortingDB;Trusted_Connection=yes;";
    SQLWCHAR outConnStr[1024];
    SQLSMALLINT outLen;

    cout << "4. Пробуем подключиться к SQL Server..." << endl;

    retcode = SQLDriverConnectW(hdbc, NULL, connStr, SQL_NTS,
        outConnStr, sizeof(outConnStr) / sizeof(outConnStr[0]),
        &outLen, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        cout << "УСПЕХ: Подключение к SQL Server установлено!" << endl;

        // Простой тестовый запрос
        SQLHSTMT hstmt;
        if (SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt) == SQL_SUCCESS) {
            cout << "Выполняем тестовый запрос..." << endl;

            if (SQLExecDirectW(hstmt, (SQLWCHAR*)L"SELECT 'Тест подключения успешен' as message", SQL_NTS) == SQL_SUCCESS) {
                SQLFetch(hstmt);
                SQLWCHAR message[100];
                SQLGetData(hstmt, 1, SQL_C_WCHAR, message, sizeof(message), NULL);
                wcout << L"Результат: " << message << endl;
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }

        // Закрываем соединение
        SQLDisconnect(hdbc);
        cout << "Соединение закрыто" << endl;
    }
    else {
        cout << "ОШИБКА: Не удалось подключиться к SQL Server. Код: " << retcode << endl;

        // Получаем детали ошибки
        SQLWCHAR sqlstate[6];
        SQLWCHAR message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER native_error;
        SQLSMALLINT length;

        SQLGetDiagRecW(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &native_error,
            message, SQL_MAX_MESSAGE_LENGTH, &length);

        char buffer[SQL_MAX_MESSAGE_LENGTH];
        WideCharToMultiByte(CP_ACP, 0, message, -1, buffer, sizeof(buffer), NULL, NULL);
        cout << "Детали ошибки: " << buffer << endl;
    }

    // Очистка
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);

    cout << "\nНажмите Enter для выхода...";
    cin.get();
    return 0;
}