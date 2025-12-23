#pragma once
#include <string>
#include <vector>

class FileDatabase {
public:
    static bool saveArray(const std::string& username,
        const std::vector<int>& original,
        const std::vector<int>& sorted);

    static std::vector<std::string> loadUserArrays(const std::string& username);

    static bool clearUserHistory(const std::string& username);

    static double testAddArrays(int count);

    static double testLoadArrays(const std::string& username);
};