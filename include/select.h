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

        static void
        add_columns_to_context(std::shared_ptr<Table> &, const std::string &, int, int, bool, ColumnContext &);

    public:
        std::shared_ptr<Table> parse_and_execute(const std::string &, TableContext &) const override;

        static void select(std::shared_ptr<Table> &, std::unique_ptr<operations::Operation>);

        static std::shared_ptr<Table> cartesian_product(const std::shared_ptr<Table> &, const std::shared_ptr<Table> &);
    };
}
