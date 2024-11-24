#include <fstream>
#include "../include/db.h"
#include "../include/token.h"
#include "../include/serializer.h"

using namespace database;

Table DataBase::execute(const std::string &target) {
    std::string_view view(target);
    auto comma = _get_command(view);
    TableContext ctx{tables_, 0};
    return comma->parse_and_execute(target, ctx);
}

std::shared_ptr<CommandExpression> DataBase::_get_command(std::string_view &view) {
    auto token = tokenize::to_lower(tokenize::get_word(view));
    SYNTAX_ASSERT(commands_.contains(token), "Неизвестная команда `" + token + "`");
    return commands_.at(token);
}

void DataBase::save_to_file(const std::string &filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    SERIAL_ASSERT(file.is_open(), "Не удалось открыть файл");

    serialization::save_int(file, tables_.size());
    for (const auto &[name, table]: tables_) {
        table.save_to_file(file);
    }
}

void DataBase::load_from_file(const std::string &filepath) {
    tables_.clear();
    std::ifstream file(filepath, std::ios::binary);
    SERIAL_ASSERT(file.is_open(), "Не удалось открыть файл");

    int size = serialization::load_int(file);
    for (int i = 0; i < size; i++) {
        auto table = Table::load_from_file(file);
        SERIAL_ASSERT(!tables_.contains(table.name()), "Считаны таблицы с одинаковым именем `" + table.name() + "`");
        tables_[table.name()] = table;
    }
}
