#pragma once

#include "command/select.h"
#include "command/create.h"
#include "command/insert.h"
#include "command/update.h"
#include "command/delete.h"

#include <list>
#include <string>
#include <map>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

class ExecutionResult {
private:
    database::Table table_{};
    int rows_num_{0};
    int cols_num_{0};
    bool is_return_{false};
    bool is_success_{false};
    std::string err_{};
public:
    ExecutionResult(const database::Table &table, bool is_return);

    ExecutionResult(const database::Table &table, const std::string &err);

    class ResultIterator {
    private:
        int from_ = 0;
        int to_ = 0;
        database::Table table_{};
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = database::Row;
        using difference_type = ptrdiff_t;
        using pointer = database::Row;
        using reference = database::Row;

        ResultIterator() = default;

        ResultIterator(const database::Table &table, int from, int to) : table_(table), from_(from), to_(to) {};

        ResultIterator &operator++();

        ResultIterator operator++(int);

        bool operator==(const ExecutionResult::ResultIterator &other) const;

        value_type operator*();
    };

    int rows_number() const;

    int columns_number() const;

    std::vector<std::pair<std::string, database::Type>> columns() const;

    bool is_success() const;

    const std::string &get_error() const;

    ResultIterator begin() const;

    ResultIterator end() const;
};

class DataBase {
private:
    std::map<std::string, database::Table> tables_;
    const static inline std::map<std::string, std::shared_ptr<database::CommandExpression>> commands_{
            {"select", std::make_shared<database::Select>()},
            {"create", std::make_shared<database::Create>()},
            {"insert", std::make_shared<database::Insert>()},
            {"update", std::make_shared<database::Update>()},
            {"delete", std::make_shared<database::Delete>()},
    };

public:

    DataBase() = default;

    ExecutionResult execute(const std::string &target);

    void save_to_file(const std::string &) const;

    void load_from_file(const std::string &);

private:
    static std::shared_ptr<database::CommandExpression> _get_command(std::string_view &);
};