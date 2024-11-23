#include <set>
#include <ranges>
#include "./../include/table.h"
#include "../include/column.h"

using namespace database;

void Table::check_valid() const {
    std::set<std::string> names;
    size_t size;
    if (!cols_.empty()) {
        size = cols_[0]->size();
    }
    for (const auto &col: cols_) {
        EXEC_ASSERT(size == col->size(),
                    "Некорректное число записей (" + std::to_string(col->size()) + "), ожидалось " +
                    std::to_string(size) + " в столбце " + col->name());
        col->check_valid();
        names.insert(col->name());
    }
    EXEC_ASSERT(names.size() == cols_.size(), "Обнаружены одинаковые имена столбцов");
}

Table &Table::operator=(Table &&other) noexcept {
    if (&other == this) {
        return *this;
    }
    name_ = std::move(other.name_);
    cols_ = std::move(other.cols_);
}

void
Table::add_columns_to_context(const std::string &alias, ColumnContext &ctx, int from, int count, bool shorty) const {
    for (auto col: std::views::counted(cols_.begin() + from, count)) {
        auto full_name = alias + "." + col->name();
        EXEC_ASSERT(!ctx.contains(full_name), "Неоднозначно определен столбец `" + full_name + "`");
        ctx[full_name] = col;
        if (shorty) {
            ctx.emplace(col->name(), col);
        }
    }
}

Table &Table::operator=(const Table &other) {
    if (&other == this) {
        return *this;
    }
    name_ = other.name_;
    cols_ = other.cols_;
    return *this;
}

const std::string &Table::name() const { return name_; }

void Table::set_name(std::string new_name) { name_ = std::move(new_name); }

void Table::add_column(const std::shared_ptr<IColumn> &col) { cols_.push_back(col); }

void Table::add_row(std::vector<std::optional<value_t>> &&row) {
    int i = 0;
    for (auto &col: cols_) {
        col->add(std::move(row[i++]));
    }
}

std::vector<std::shared_ptr<IColumn>> Table::get_columns() const { return cols_; }

