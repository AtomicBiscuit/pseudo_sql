#include "../../include/data/column.h"

using namespace database;

std::shared_ptr<IColumn> database::IColumn::load_from_file(std::ifstream &file) {
    auto name = serialization::load_str(file);
    Type type = static_cast<Type>(serialization::load_int(file));
    std::shared_ptr<IColumn> col;

    SERIAL_ASSERT(type == Type::Integer or type == Type::String or type == Type::Bool or type == Type::Bytes,
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

bool IColumn::operator==(const IColumn &other) const {
    if (other.type_ != type_ or other.is_unique_ != is_unique_ or other.is_default_ != is_default_ or
        other.name_ != name_ or other.is_autoinc_ != is_autoinc_ or other.default_value_ != default_value_ or
        other.size() != size()) {
        return false;
    }
    for (int i = 0; i < size(); i++) {
        if (other.get_value(i) != get_value(i)) {
            return false;
        }
    }
    return true;
}
