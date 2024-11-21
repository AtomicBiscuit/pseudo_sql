#pragma once

#include "operations.h"
#include "expression.h"

#include <memory>

namespace database {
    class Create : public database::CommandExpression {
    private:

    public:
        std::shared_ptr<Table> parse_and_execute(const std::string &, TableContext &) const override;
    };
}
