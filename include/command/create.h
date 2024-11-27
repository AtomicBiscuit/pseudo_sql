#pragma once

#include "../operations.h"
#include "../expression.h"
#include "../data/column.h"

#include <memory>

namespace database {
    class Create : public database::CommandExpression {
    public:
        Table parse_and_execute(const std::string &, TableContext &) const override;

        static Table create(const std::string &, const std::vector<std::shared_ptr<IColumn>> &, TableContext &);

    private:
        static std::tuple<bool, bool, bool> _parse_attributes(std::string_view &);

        static std::shared_ptr<IColumn> _parse_and_create_col(std::string_view &);
    };
}
