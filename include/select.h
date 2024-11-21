#pragma once

#include "operations.h"
#include "expression.h"

#include <memory>

namespace database {
    class Select : public database::CommandExpression {
    private:
        static std::vector<std::pair<std::unique_ptr<operations::Operation>, std::string>>
        _resolve_column_expr(const std::string &, ColumnContext &);

        static std::tuple<std::shared_ptr<Table>, ColumnContext>
        _resolve_table_expr(const std::string &, TableContext &);

    public:
        std::shared_ptr<Table> parse_and_execute(const std::string &, TableContext &) const override;
    };
}
