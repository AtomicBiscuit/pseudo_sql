#pragma once

#include "operations.h"
#include "expression.h"

#include <memory>

namespace database {
    class Create : public database::CommandExpression {

        static std::tuple<bool, bool, bool> _parse_attributes(const std::string &);

        static std::shared_ptr<IColumn> _parse_and_create_col(std::string_view &);

    public:
        std::shared_ptr<Table> parse_and_execute(const std::string &, TableContext &) const override;
    };
}
