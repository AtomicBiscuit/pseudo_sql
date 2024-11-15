#pragma once

#include <variant>
#include <vector>
#include <string>
#include <sstream>

namespace database {
    enum class Type {
        Integer,
        Boolean,
        String,
        Bytes,
        None,
    };

    static inline std::string type_to_str(Type t) {
        switch (t) {
            case (Type::Integer):
                return "int32";
            case (Type::Boolean):
                return "bool";
            case (Type::String):
                return "string";
            case (Type::Bytes):
                return "bytes";
            default:
                return "none";
        }
    }

    using value_t = std::variant<int, bool, std::string, std::vector<bool>>;

    static inline std::string value_to_string(const value_t &val, Type type) {
        if (type == database::Type::Integer)
            return std::to_string(get<int>(val));
        if (type == database::Type::Boolean)
            return std::to_string(get<bool>(val));
        if (type == database::Type::String)
            return get<std::string>(val);
        std::stringstream out;
        out << "0x";
        const auto &bytes = get<std::vector<bool>>(val);
        for (size_t i = 0; i < bytes.size(); i += 4) {
            out << std::hex << 8 * bytes[i] + 4 * bytes[i + 1] + 2 * bytes[i + 2] + bytes[i + 3];
        }
        return out.str();
    }

    class IColumn {
        Type type_;
        std::string name_;
    public:

        IColumn(Type type, const std::string &name) : type_(type), name_(name) {};

        Type type() const { return type_; }

        const std::string &name() const { return name_; }

        virtual value_t get_value(int row) const = 0;
    };

    template<typename T> requires(std::convertible_to<T, value_t>)
    class Column : public IColumn {
        std::vector<T> rows_;
    public:
        Column(Type type, const std::string &name) : IColumn(type, name) {};

        Column(const Column &other) : rows_(other.rows_) {};

        value_t get_value(int row) const override {
            return rows_[row];
        }
    };
}