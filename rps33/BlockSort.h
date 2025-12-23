#pragma once

#include <vector>

namespace sort {

    class BlockSorter {
    public:
        // Сортирует вектор целых чисел на месте (адаптированная версия)
        static void sort(std::vector<int>& data);

        // Проверка сортировки
        static bool isSorted(const std::vector<int>& data);
    };

} // namespace sort