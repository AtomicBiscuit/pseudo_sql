#pragma once

#include "db.h"

#include <iostream>
#include <iomanip>

using namespace std::string_literals;

struct Colors {
    const static inline std::string blue = "\033[1;34m",
            cyan = "\033[1;36m",
            green = "\033[1;32m",
            yellow = "\033[1;33m",
            red = "\033[1;31m",
            black = "\033[1;30m",
            clear = "\033[0m";
};

std::string truncate(std::string str, int width) {
    if (str.size() > width)
        return str.substr(0, std::max(0, width - 3)) + "...";
    return str;
}

std::string repeat(const std::string &s, int n) {
    std::string repeat;
    for (int i = 0; i < n; i++)
        repeat += s;
    return repeat;
}

void print_table(const ExecutionResult &table, int width = 20) {
    if (not table.is_success()) {
        std::cerr << Colors::red << table.get_error() << Colors::clear << "\n";
    }

    int col_num = table.columns_number();

    auto cols = table.columns();

    if (col_num == 0) {
        return;
    }

    std::cout << Colors::cyan << "\n╔";
    for (int i = 0; i < col_num; i++) {
        std::cout << repeat("═", width) << (i == col_num - 1 ? "╗" : "╦");
    }
    std::cout << '\n' << std::string("║");

    for (auto &[name, type]: cols) {
        std::cout << Colors::blue << std::setw(width) << std::setfill(' ') << truncate(name, width) << Colors::cyan
                  << "║";
    }

    std::cout << "\n║";

    for (auto &[name, type]: cols) {
        std::cout << Colors::blue << std::setw(width) << std::setfill(' ') << database::type_to_str(type)
                  << Colors::cyan << "║";
    }

    std::cout << Colors::cyan << "\n╠";
    for (int i = 0; i < col_num; i++) {
        std::cout << repeat("═", width) << (i == col_num - 1 ? "╣" : "╬");
    }
    std::cout << '\n';

    int ind = 0;
    for (auto row: table) {
        std::cout << Colors::cyan << "║";
        ++ind;
        for (auto &[col, type]: cols) {
            std::cout << Colors::green << std::setw(width) << std::setfill(' ')
                      << truncate(database::value_to_string(row[col], type), width) << Colors::cyan <<
                      "║";

        }
        std::cout << '\n';
    }
    std::cout << Colors::cyan << "╚";
    for (int i = 0; i < col_num; i++) {
        std::cout << repeat("═", width) << (i == col_num - 1 ? "╝" : "╩");
    }
    std::cout << '\n' << Colors::clear;
}