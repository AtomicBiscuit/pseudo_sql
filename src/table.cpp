#include <set>
#include "./../include/table.h"

using namespace database;

void Table::check_valid() const {
    std::set<std::string> names;
    size_t size;
    if (!cols_.empty()) {
        size = cols_[0]->size();
    }
    for (const auto &col: cols_) {
        if (size != col->size()) {
            throw execution_error("Некорректное число записей(" + std::to_string(col->size()) + ", ожидалось " +
                                  std::to_string(size) + ") в столбце " + col->name());
        }
        col->check_valid();
        names.insert(col->name());
    }
    if (names.size() != cols_.size()) {
        throw execution_error("Обнаружены одинаковые имена столбцов");
    }
}

std::shared_ptr<Table> Table::copy() const {
    auto res = std::make_shared<Table>(name_);
    for (auto &col: cols_) {
        res->add_column(col->multicopy(1, true));
    }
    return res;
}


