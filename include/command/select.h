#pragma once

#include "../operations.h"
#include "../expression.h"

#include <memory>

namespace database {
    class Select : public database::CommandExpression {
    public:
        Table parse_and_execute(const std::string &, TableContext &) const override;


        static Table select(Table &, std::unique_ptr<operations::Operation>,
                            std::vector<std::pair<std::unique_ptr<operations::Operation>, std::string>> &,
                            const std::string &);

        static void
        cartesian_product_on_condition(Table &, const Table &, const Table &, std::unique_ptr<operations::Operation>);

    private:
        static std::vector<std::pair<std::unique_ptr<operations::Operation>, std::string>>
        _resolve_column_expr(const std::string &, ColumnContext &);

        static std::tuple<Table, ColumnContext> _resolve_table_expr(const std::string &, TableContext &);

        static std::unique_ptr<operations::Operation>
        _save_on_condition(Table &, std::unique_ptr<operations::Operation>);
    };
}
