#include <iostream>
#include <memory>
#include "include/token.h"
#include "include/select.h"
#include "include/parser.h"

static tokenize::CommandRegister sel("select", std::make_shared<database::Select>());

int main() {
    //tokenize::Parser::get_parser().parse_command("seLect x1 + 5, 6 from users join pepegas");
    database::Column<int> col(database::Type::Integer, "name");
    while (true) {
        try {
            std::string s;
            std::getline(std::cin, s, ';');
            tokenize::Parser::get_parser().execute_command(s);
        } catch (syntax_error &err) {
            std::cerr << "Syntax error: " << err.what() << std::endl;
        } catch (execution_error &err) {
            std::cerr << "Execution error: " << err.what() << std::endl;
        }
    }
    return 0;
}
