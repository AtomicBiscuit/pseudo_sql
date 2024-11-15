#pragma once

#include <list>
#include <string>
#include <map>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>
#include "expression.h"

namespace tokenize {
    class Parser {
    private:
        std::map<std::string, std::shared_ptr<database::CommandExpression>> command_;
    public:
        std::unique_ptr<database::Operation> parse_command(const std::string &) const;

        std::shared_ptr<database::CommandExpression> get_command(std::stringstream &) const;

        static inline Parser &get_parser() {
            static Parser _parser;
            return _parser;
        }

        void register_command(const std::string &, std::shared_ptr<database::CommandExpression>);
    };

    class CommandRegister {
    public:
        CommandRegister(const std::string &name, std::shared_ptr<database::CommandExpression> com) {
            std::cout << name << std::endl;
            Parser::get_parser().register_command(name, std::move(com));
        }
    };
}

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}