#pragma once

#include "operations.h"
#include "expression.h"

#include <memory>

namespace database {
    class Insert : public database::CommandExpression {
        static std::vector<std::optional<value_t>> _parse_linear(std::string_view &, const std::shared_ptr<Table> &);

        static std::vector<std::optional<value_t>> _parse_by_names(std::string_view &, const std::shared_ptr<Table> &);

        static std::vector<std::optional<value_t>> _parse_value(const std::string &, const std::shared_ptr<Table> &);

    public:
        std::shared_ptr<Table> parse_and_execute(const std::string &, TableContext &) const override;
    };
}
