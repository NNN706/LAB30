#include "FileDatabase.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <ctime>

bool FileDatabase::saveArray(const std::string& username,
    const std::vector<int>& original,
    const std::vector<int>& sorted) {
    try {
        std::ofstream file(username + "_arrays.txt", std::ios::app);
        if (!file.is_open()) return false;

        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        char timeStr[100];
        ctime_s(timeStr, sizeof(timeStr), &time);

        std::string timeStrClean = timeStr;
        if (!timeStrClean.empty() && timeStrClean.back() == '\n') {
            timeStrClean.pop_back();
        }

        file << "[" << timeStrClean << "] ";
        file << "Исходный: ";
        for (size_t i = 0; i < original.size(); ++i) {
            file << original[i];
            if (i < original.size() - 1) file << " ";
        }

        file << " | Сортированный: ";
        for (size_t i = 0; i < sorted.size(); ++i) {
            file << sorted[i];
            if (i < sorted.size() - 1) file << " ";
        }

        file << "\n";
        file.close();
        return true;
    }
    catch (...) {
        return false;
    }
}

std::vector<std::string> FileDatabase::loadUserArrays(const std::string& username) {
    std::vector<std::string> result;
    std::ifstream file(username + "_arrays.txt");

    if (!file.is_open()) return result;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            result.push_back(line);
        }
    }

    file.close();
    return result;
}

bool FileDatabase::clearUserHistory(const std::string& username) {
    std::ofstream file(username + "_arrays.txt", std::ios::trunc);
    if (!file.is_open()) return false;
    file.close();
    return true;
}

double FileDatabase::testAddArrays(int count) {
    auto start = std::chrono::high_resolution_clock::now();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);

    for (int i = 0; i < count; i++) {
        int size = dis(gen) % 10 + 5;
        std::vector<int> arr(size);
        for (int& num : arr) num = dis(gen);
        saveArray("test_user", arr, arr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return diff.count();
}

double FileDatabase::testLoadArrays(const std::string& username) {
    auto start = std::chrono::high_resolution_clock::now();

    auto arrays = loadUserArrays(username);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return diff.count();
}