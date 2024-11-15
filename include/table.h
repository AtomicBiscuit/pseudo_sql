#pragma once

#include <variant>
#include <vector>
#include <string>

namespace database {
    enum class Type {
        Integer,
        Boolean,
        String,
        Bytes,
        None,
    };

    static std::string type_to_str(Type t) {
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

    class IColumn {
        Type type_;
        std::string name_;
    public:
        using value_t = std::variant<int, bool, std::string, std::vector<bool>>;

        IColumn(Type t, const std::string &n) : type_(t), name_(n) {};

        Type type() const { return type_; }

        const std::string &name() const { return name_; }

        virtual value_t get_value(int row) const = 0;
    };

    template<typename T>
    class Column : public IColumn {
        std::vector<T> rows_;
    public:
        Column() = default;

        Column(const Column &other) : rows_(other.rows_) {};

        value_t get_value(int row) const override {
            return rows_[row];
        }
    };
}