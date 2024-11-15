#include <sstream>
#include <vector>
#include <ranges>
#include "../include/select.h"

using namespace std::string_literals;

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
        std::vector<std::string> names;
        int col_num = 0;
        for (auto &full_column: tokenize::Parser::clear_parse(cols.str(), ",", false)) {
            auto parts = tokenize::Parser::clear_parse(full_column, "as", true);
            if (parts.size() > 2) {
                throw syntax_error("Обнаружено несколько вхождений ключевого слова `as`");
            }
            database::ColumnContext ctx{};
            columns.push_back(database::ColumnExpression::parse(parts[0], ctx));
            std::string name = "column " + std::to_string(++col_num);
            if (parts.size() == 1) {
                database::FieldOperation *column;
                if ((column = dynamic_cast<database::FieldOperation *>(columns.back().get()))) {
                    name = Parser::clear_parse(column->name(), ".", false).back();
                }
            } else {
                auto view = std::string_view(parts[1]);
                while (!view.empty() and " \n\r"s.contains(view[0])) { view.remove_prefix(1); }
                name = Token::get_name(view);
                for (auto c: view) {
                    if (c != ' ' and c != '\n' and c != '\r') {
                        throw syntax_error("Непредвиденный литерал `" + std::string(view) + "`");
                    }
                }
            }
            names.push_back(name);
            database::value_t val = columns.back()->eval(0);
            database::Type type = columns.back()->type();
            std::cout << parts[0] << " | name = " << name << " | value = " << database::value_to_string(val, type);
            std::cout << std::endl;
        }
        // TableExpression::parse(in.str());
        return nullptr;
    }
}