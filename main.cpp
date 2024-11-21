#include <iostream>
#include <memory>
#include "include/token.h"
#include "include/select.h"
#include "include/db.h"


int main() {
    //tokenize::Parser::get_parser().parse_command("seLect x1 + 5, 6 from users join pepegas");
    DataBase db;
    while (true) {
        try {
            std::string s;
            std::getline(std::cin, s, ';');
            db.execute(s);
        } catch (syntax_error &err) {
            std::cerr << "Syntax error: " << err.what() << std::endl;
        } catch (execution_error &err) {
            std::cerr << "Execution error: " << err.what() << std::endl;
        }
    }
    return 0;
}
