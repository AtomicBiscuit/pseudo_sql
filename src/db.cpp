#include "../include/db.h"
#include "../include/token.h"
#include <stdexcept>
#include <utility>
#include <sstream>

using namespace tokenize;


std::shared_ptr<database::Table> DataBase::execute(const std::string &target) {
    std::stringstream in(target);
    auto comma = _get_command(in);
    database::TableContext ctx;
    return comma->parse_and_execute(in.str(), tables_);;
}

std::shared_ptr<database::CommandExpression> DataBase::_get_command(std::stringstream &in) {
    std::string token;
    in >> token;
    if (commands_.contains(to_lower(token))) {
        return commands_.at(to_lower(token));
    }
    throw syntax_error("Неизвестная команда `" + token + "`");
}
