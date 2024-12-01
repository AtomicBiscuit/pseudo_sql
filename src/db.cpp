#include <fstream>
#include "../include/db.h"
#include "../include/token.h"

using namespace database;

ExecutionResult DataBase::execute(const std::string &target) {
    TableContext ctx{tables_, 0};
    std::string_view view(target);
    try {
        auto command = _get_command(view);
        auto table = command->parse_and_execute(target, ctx);
        return {table, dynamic_cast<Select *>(command.get()) != nullptr};
    } catch (syntax_error &err) {
        return {Table(), "Syntax error: "s + err.what()};
    } catch (execution_error &err) {
        return {Table(), "Execution error: "s + err.what()};
    }
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

ExecutionResult::ResultIterator ExecutionResult::end() const {
    return {table_, rows_num_, rows_num_};
}

ExecutionResult::ResultIterator ExecutionResult::begin() const {
    if (!is_return_ or !is_success_) {
        return end();
    }
    return {table_, 0, rows_num_};
}

const std::string &ExecutionResult::get_error() const {
    return err_;
}

bool ExecutionResult::is_success() const {
    return is_success_;
}

std::vector<std::pair<std::string, database::Type>> ExecutionResult::columns() const {
    if (!is_return_ or !is_success_) {
        return {};
    }
    std::vector<std::pair<std::string, database::Type>> cols;
    for (auto &col: table_.columns()) {
        cols.emplace_back(col->name(), col->type());
    }
    return cols;
}

int ExecutionResult::columns_number() const {
    return cols_num_;
}

int ExecutionResult::rows_number() const {
    return rows_num_;
}

ExecutionResult::ExecutionResult(const Table &table, bool is_return) : table_(table), is_return_(is_return),
                                                                       is_success_(true) {
    if (is_return_) {
        cols_num_ = table.columns().size();
        rows_num_ = cols_num_ == 0 ? 0 : table.columns()[0]->size();
    }
}

ExecutionResult::ExecutionResult(const Table &table, const std::string &err) : table_(table), err_(err) {}

ExecutionResult::ResultIterator &ExecutionResult::ResultIterator::operator++() {
    if (from_ < to_) {
        from_++;
    }
    return *this;
}

ExecutionResult::ResultIterator ExecutionResult::ResultIterator::operator++(int) {
    if (from_ < to_) {
        return {table_, from_++, to_};
    }
    return {table_, from_, to_};;
}

bool ExecutionResult::ResultIterator::operator==(const ExecutionResult::ResultIterator &other) const {
    return from_ == other.from_ and to_ == other.to_;
}

ExecutionResult::ResultIterator::value_type ExecutionResult::ResultIterator::operator*() {
    return table_.row(from_);
}
