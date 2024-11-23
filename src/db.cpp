#include "../include/db.h"
#include "../include/token.h"

using namespace tokenize;


database::Table DataBase::execute(const std::string &target) {
    std::string_view view(target);
    auto comma = _get_command(view);
    database::TableContext ctx{tables_, 0};
    return comma->parse_and_execute(target, ctx);
}

std::shared_ptr<database::CommandExpression> DataBase::_get_command(std::string_view &view) {
    auto token = to_lower(get_word(view));
    SYNTAX_ASSERT(commands_.contains(token), "Неизвестная команда `" + token + "`");
    return commands_.at(token);
}
