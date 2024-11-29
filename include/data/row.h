#pragma once

#include "value.h"
#include <map>

namespace database {
    class Row {
    private:
        std::map<std::string, database::value_t> row_;
    public:
        explicit Row(const std::map<std::string, database::value_t> &row) : row_(row) {};

        Row(Row &&other) noexcept: row_(std::move(other.row_)) {};

        const value_t &operator[](const std::string &column) const { return row_.at(column); }
    };
}