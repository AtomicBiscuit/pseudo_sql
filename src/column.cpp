#include "../include/column.h"

using namespace database;

std::shared_ptr<IColumn> database::IColumn::load_from_file(std::ifstream &file) {
    auto name = serialization::load_str(file);
    Type type = static_cast<Type>(serialization::load_int(file));
    std::shared_ptr<IColumn> col;

    SERIAL_ASSERT(type == Type::Integer or type == Type::String or type == Type::Boolean or type == Type::Bytes,
                "Попытка считать неизвестный тип данных");
    if (type == Type::Integer) {
        col = std::make_shared<Column<int>>(type, name);
    } else if (type == Type::String) {
        col = std::make_shared<Column<std::string>>(type, name);
    } else if (type == Type::Bytes) {
        col = std::make_shared<Column<std::vector<bool>>>(type, name);
    } else {
        col = std::make_shared<Column<bool>>(type, name);
    }
    col->_load_from_file_impl(file);
    return col;
}
