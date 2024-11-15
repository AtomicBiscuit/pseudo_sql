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

static bool skip_until(std::string_view &view, char tar) {
    while (!view.empty() and view[0] != tar) {
        view.remove_prefix(1);
    }
    if (view.empty()) {
        return false;
    } else {
        view.remove_prefix(1);
        return true;
    }
}

std::vector<std::string> Parser::clear_parse(const std::string &str, const std::string &del, bool is_separated) {
    if (del.contains('"')) {
        return {};
    }
    const std::string sep = " \n\r, ";
    std::string_view view(str);
    bool is_last_sep = true;

    std::vector<std::string> res;
    auto start = view.begin();
    while (!view.empty()) {
        if (view[0] == '"') {
            view.remove_prefix(1);
            skip_until(view, '"');
            is_last_sep = false;
            continue;
        }
        if (view.starts_with(del)) {
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
        if (is_separated)
            is_last_sep = sep.contains(view[0]);
        view.remove_prefix(1);
    }
    res.emplace_back(start, view.begin());
    return res;
}
