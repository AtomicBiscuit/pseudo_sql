#pragma once

#include "operations.h"
#include "expression.h"
#include "column.h"

#include <memory>

namespace database {
    class Delete : public database::CommandExpression {

    public:
        Table parse_and_execute(const std::string &, TableContext &) const override;

        static Table delete_impl(Table, std::unique_ptr<operations::Operation> &);

    private:
        static std::vector<std::pair<std::shared_ptr<IColumn>, std::unique_ptr<operations::Operation>>>
        _parse_assignments(const std::string &, ColumnContext &);

    };
}