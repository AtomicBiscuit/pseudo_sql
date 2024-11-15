#pragma once

#include "token.h"
#include "operations.h"
#include <string>
#include <sstream>

namespace database {

    class TableExpression {

    };

    using ColumnContext = std::map<std::string, std::shared_ptr<database::IColumn>>;

    class ColumnExpression {
    public:
        static std::unique_ptr<Operation> parse(std::stringstream &, ColumnContext &);
    };

    class CommandExpression {
    public:
        virtual std::unique_ptr<Operation> parse(std::stringstream &) const = 0;
    };
}