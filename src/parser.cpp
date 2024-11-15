#include "../include/parser.h"
#include <stdexcept>
#include <utility>
#include <sstream>

using namespace tokenize;


std::unique_ptr<database::Operation> Parser::parse_command(const std::string &target) const {
    std::stringstream in(target);
    auto comma = get_command(in);
    comma->parse(in);
    return nullptr;
}

std::shared_ptr<database::CommandExpression> Parser::get_command(std::stringstream &in) const {
    std::string token;
    in >> token;
    if (command_.contains(to_lower(token))) {
        return command_.at(to_lower(token));
    }
    throw syntax_error("Неизвестная команда `" + token + "`");
}

void Parser::register_command(const std::string &name, std::shared_ptr<database::CommandExpression> comma) {
    if (command_.contains(name)) {
        throw syntax_error("Команда `" + name + "` уже определена");
    }
    command_[name] = std::move(comma);
}
