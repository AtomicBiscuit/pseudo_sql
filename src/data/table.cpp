#include <set>
#include <ranges>
#include <fstream>
#include "../../include/data/table.h"

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

void Table::add_column(const std::shared_ptr<IColumn> &col) { cols_.push_back(col); }

void Table::add_row(std::vector<std::optional<value_t>> &&row) {
    int i = 0;
    for (auto &col: cols_) {
        col->check_able_to_insert(row[i++]);
    }
    i = 0;
    for (auto &col: cols_) {
        col->add(std::move(row[i++]));
    }
}

std::vector<std::shared_ptr<IColumn>> Table::columns() const { return cols_; }

void Table::save_to_file(std::ofstream &file) const {
    serialization::save_str(file, name_);
    serialization::save_int(file, cols_.size());
    for (auto &col: cols_) {
        col->save_to_file(file);
    }
}

Table Table::load_from_file(std::ifstream &file) {
    auto name = serialization::load_str(file);
    int size = serialization::load_int(file);
    Table temp(name);
    for (int i = 0; i < size; i++) {
        temp.add_column(IColumn::load_from_file(file));
    }
    temp.check_valid();
    return temp;
}

Row Table::row(int num) const {
    if (num < 0 or num > cols_[0]->size()) {
        throw std::out_of_range("Не найдена строка с номером " + std::to_string(num));
    }
    std::map<std::string, value_t> res;
    for (auto &col: cols_) {
        res[col->name()] = col->get_value(num);
    }
    return Row(res);
}

void Table::merge(Table &other) const {
    for (int i = 0; i < cols_.size(); i++) {
        cols_[i]->merge(other.cols_[i]);
    }
}

