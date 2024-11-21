#pragma once

#include "table.h"
#include "operations.h"

#include <string>
#include <sstream>

namespace database {
    std::tuple<std::shared_ptr<Table>, std::string> get_table_from_expression(const std::string &, TableContext &);

    std::unique_ptr<operations::Operation> build_execution_tree_from_expression(const std::string &, ColumnContext &);

    class CommandExpression {
    public:
        virtual std::shared_ptr<Table> parse_and_execute(const std::string &, TableContext &) const = 0;

        virtual ~CommandExpression() = default;
    };
}