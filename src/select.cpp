#include <sstream>
#include <vector>
#include <ranges>
#include <span>
#include "../include/select.h"

namespace tokenize {
    std::unique_ptr<database::Operation> Select::parse(std::stringstream &in) const {
        std::stringstream cols;
        std::string buf;
        do {
            cols << buf << " ";
            in >> buf;
        } while (to_lower(buf) != "from" and !in.eof());

        if (to_lower(buf) != "from") {
            throw syntax_error("Ожидалось ключевое слово from");
        }

        std::vector<std::unique_ptr<database::Operation>> columns;
        std::string st = cols.str();
        for (std::span<const char> column: st | std::views::split(',')) {
            std::cout << std::string_view{column} << std::endl;
            std::stringstream IN({column.begin(), column.end()});
            database::ColumnContext ctx{};
            columns.push_back(database::ColumnExpression::parse(IN, ctx));
        }
        // TableExpression::parse(in.str());
        return nullptr;
    }
}