#pragma once

#include "column.h"

#include <variant>
#include <vector>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_set>
#include <optional>

namespace database {
    class Table {
        std::string name_;
        std::vector<std::shared_ptr<IColumn>> cols_;
    public:
        Table() = default;

        explicit Table(const std::string &name) : name_(name) {};

        Table(const Table &other) : name_(other.name_), cols_(other.cols_) {};

        Table(Table &&other) noexcept: name_(std::move(other.name_)), cols_(std::move(other.cols_)) {};

        Table &operator=(const Table &other);

        Table &operator=(Table &&other) noexcept;

        std::vector<std::shared_ptr<IColumn>> get_columns() const;

        const std::string &name() const;

        void set_name(std::string new_name);

        void add_columns_to_context(const std::string &, ColumnContext &, int, int, bool) const;

        void add_column(const std::shared_ptr<IColumn> &col);

        void add_row(std::vector<std::optional<value_t>> &&row);

        void check_valid() const;

        void save_to_file(std::ofstream &) const;

        static Table load_from_file(std::ifstream &) ;
    };

    struct TableContext {
        std::map<std::string, database::Table> &tables;
        int number;
    };

}