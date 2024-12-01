#include "include/db.h"
#include "include/print.h"


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

insert (name = "Jojo",       state
 = true
	,
	vars= 0x001
) to table;

update table set i_col=|name| + |vars|, vars=0xDEAD where i_col != 100;

select a.b + "____" + b.name as b_join_name, a.c, a.c / b.i_col, b.i_col, b.name, b.state, b.vars as very__long__name__yes
from (select "a" + "_" + name as b, 7777 + table.i_col as c from table where true) as a
	join table as b on true
where true;

select i_col - 4 * 3 / (|"12"|%(|("123")|%|"123456"|))+1 as num_____1,
         0xabc > 0x123   	as book_4,
		 name + "_" + "777" as t_
from table
where i_col >= 6;)";


int main() {
    DataBase db;

    std::string command_str;

    std::stringstream example_stream(example);

    while (std::getline(example_stream, command_str, ';')) {
        auto res = db.execute(command_str);
        print_table(res, 20, 16); // Второй параметр - ширина столбцов, третий - максимальное число выводимых строк
    }

    db.save_to_file("dump.bin");
    db.load_from_file("dump.bin");

    while (true) {
        std::getline(std::cin, command_str, ';');
        auto res = db.execute(command_str);
        print_table(res, 20, 100);
    }
    return 0;
}