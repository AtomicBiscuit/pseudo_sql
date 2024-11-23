#pragma once

#include "table.h"
#include "operations.h"

#include <string>
#include <sstream>

namespace database {
    std::tuple<Table, std::string> get_table_from_expression(const std::string &in, TableContext &ctx);

    std::unique_ptr<operations::Operation> build_execution_tree_from_expression(const std::string &, ColumnContext &);

    class CommandExpression {
    public:
        virtual Table parse_and_execute(const std::string &, TableContext &) const = 0;

        virtual ~CommandExpression() = default;
    };
}