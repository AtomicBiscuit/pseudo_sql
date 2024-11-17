#pragma once

#include "token.h"
#include "operations.h"

#include <string>
#include <sstream>

namespace database {
    std::unique_ptr<Operation> build_execution_tree_from_expression(const std::string &, ColumnContext &);

    std::tuple<std::shared_ptr<Table>, std::string> get_table_from_expression(const std::string &, TableContext &);

    class CommandExpression {
    public:
        virtual std::shared_ptr<Table> parse_and_execute(const std::string &, TableContext &) const = 0;

        virtual ~CommandExpression() = default;
    };
}