#pragma once

#include "operations.h"
#include "expression.h"

#include <memory>

namespace database {
    class Insert : public database::CommandExpression {
    public:
        Table parse_and_execute(const std::string &, TableContext &) const override;

    private:
        static std::vector<std::optional<value_t>> _parse_linear(std::string_view &view, const Table &table);

        static std::vector<std::optional<value_t>> _parse_by_names(std::string_view &view, const Table &table);

        static std::vector<std::optional<value_t>> _parse_value(const std::string &value_str, const Table &table);
    };
}
