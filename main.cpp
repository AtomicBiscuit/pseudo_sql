#include "include/db.h"

#include <iostream>
#include <iomanip>

using namespace std::string_literals;

void print_table(std::shared_ptr<database::Table> &table) {
    if (!table) {
        return;
    }
    int width = 24;
    int size = table->get_columns().empty() ? 0 : table->get_columns()[0]->size();
    std::cout << "\033[1m";
    std::cout << std::setw((width + 1) * table->get_columns().size() + 2) << std::setfill('-') << '\n';
    std::cout << '|';
    for (auto &col: table->get_columns()) {
        std::cout << std::setw(width) << std::setfill(' ') << col->name() << '|';
    }
    std::cout << "\n";
    std::cout << "|";
    for (auto &col: table->get_columns()) {
        std::cout << std::setw(width) << std::setfill(' ') << database::type_to_str(col->type()) << '|';
    }
    std::cout << '\n';
    std::cout << std::setw((width + 1) * table->get_columns().size() + 2) << std::setfill('-') << '\n';
    for (int i = 0; i < size; i++) {
        std::cout << "|";
        for (auto &col: table->get_columns()) {
            std::cout << std::setw(width) << std::setfill(' ')
                      << database::value_to_string(col->get_value(i), col->type()) << '|';
        }
        std::cout << '\n';
    }
    std::cout << std::setw((width + 1) * table->get_columns().size() + 2) << std::setfill('-') << '\n';
    std::cout << "\033[0m";
}

int main() {
    DataBase db;
    std::string example = R"(
create table table(
	{unique, key, autoincrement}i_col:InTeger=5,
	name : string ,
	state: bool,
	vars:  bytes
);

insert (, "Biba",    true, 0x55) to table;

insert (, "Alice",    false, 0xabc) to table;

insert (100, "Handra",    false, 0xfeed) to table;

select a.b + "__joined__" + b.name as b_join_name, a.c, a.c / b.i_col, b.i_col, b.name, b.state, b.vars
from (select "a" + "_" + name as b, 7777 + table.i_col as c from table where true) as a
	join table as b on true
where true;)";
    std::stringstream example_stream(example);
    std::string s;
    while (std::getline(example_stream, s, ';')) {
        auto res = db.execute(s);
        print_table(res);
    }

    while (true) {
        try {
            std::getline(std::cin, s, ';');
            auto res = db.execute(s);
            print_table(res);
        } catch (syntax_error &err) {
            std::cerr << "Syntax error: " << err.what() << std::endl;
        } catch (execution_error &err) {
            std::cerr << "Execution error: " << err.what() << std::endl;
        }
    }
    return 0;
}