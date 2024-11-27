#pragma once

#include "../operations.h"
#include "../expression.h"

#include <memory>

namespace database {
    class Select : public database::CommandExpression {
    public:
        Table parse_and_execute(const std::string &, TableContext &) const override;

        static void select(Table &, std::unique_ptr<operations::Operation>);

        static Table select(Table &, std::unique_ptr<operations::Operation>,
                            std::vector<std::pair<std::unique_ptr<operations::Operation>, std::string>> &,
                            const std::string &);

        static Table cartesian_product(const Table &, const Table &);

    private:
        static std::vector<std::pair<std::unique_ptr<operations::Operation>, std::string>>
        _resolve_column_expr(const std::string &, ColumnContext &);

        static std::tuple<Table, ColumnContext> _resolve_table_expr(const std::string &, TableContext &);
    };
}
