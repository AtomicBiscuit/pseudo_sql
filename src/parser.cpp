#include "../include/parser.h"
#include <stdexcept>
#include <utility>
#include <sstream>

using namespace tokenize;


std::unique_ptr <database::Operation> Parser::execute_command(const std::string &target) const {
    std::stringstream in(target);
    auto comma = get_command(in);
    database::TableContext ctx;
    comma->parse_and_execute(in.str(), ctx);
    return nullptr;
}

std::shared_ptr <database::CommandExpression> Parser::get_command(std::stringstream &in) const {
    std::string token;
    in >> token;
    if (command_.contains(to_lower(token))) {
        return command_.at(to_lower(token));
    }
    throw syntax_error("Неизвестная команда `" + token + "`");
}

void Parser::register_command(const std::string &name, std::shared_ptr <database::CommandExpression> comma) {
    if (command_.contains(name)) {
        throw syntax_error("Команда `" + name + "` уже определена");
    }
    command_[name] = std::move(comma);
}

std::vector <std::string> Parser::clear_parse(const std::string &str, const std::string &del, bool is_separated) {
    if (del.starts_with('\\') or del.starts_with('"') or del.starts_with('(')) {
        throw std::domain_error("clear_parse не может обработать разделитель: `" + del + "`");
    }
    const std::string sep = " \n\r\t, ";
    std::string_view view(str);
    bool is_last_sep = true;
    bool skip = false;
    int level = 0;

    std::vector <std::string> res;
    auto start = view.begin();
    while (!view.empty()) {
        if (view[0] == '\\') {
            view.remove_prefix(1 + (view.size() > 1));
        } else if (view[0] == '"') {
            view.remove_prefix(1);
            skip = !skip;
            is_last_sep = false;
        } else if (skip) {
            view.remove_prefix(1);
        } else {
            level += view[0] == '(' ? 1 : (view[0] == ')' ? -1 : 0);
            if (level == 0 and to_lower(std::string(view)).starts_with(del)) {
                if (is_separated and is_last_sep and (view.size() == del.size() or sep.contains(view[del.size()]))) {
                    is_last_sep = false;
                    res.emplace_back(start, view.begin());
                    view.remove_prefix(del.size());
                    start = view.begin();
                    continue;
                } else if (!is_separated) {
                    res.emplace_back(start, view.begin());
                    view.remove_prefix(del.size());
                    start = view.begin();
                    continue;
                }
            }
            if (level == 0 and is_separated)
                is_last_sep = sep.contains(view[0]);
            view.remove_prefix(1);
        }
    }
    res.emplace_back(start, view.begin());
    return res;
}
