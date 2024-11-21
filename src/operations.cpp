#include "../include/operations.h"

using namespace database::operations;
using namespace std::string_literals;

void BinaryOperation::bind(std::vector<std::unique_ptr<Operation>> &ops) {
    auto f = std::move(ops.back());
    ops.pop_back();
    f->bind(ops);

    auto s = std::move(ops.back());
    ops.pop_back();
    s->bind(ops);

    arg1_ = std::move(f);
    arg2_ = std::move(s);
    type_ = arg1_->type();

    if (arg1_->type() != arg2_->type() or type_ == Type::Bytes) {
        _throw();
    }
    if (type_ == Type::String and op_ != '+') {
        _throw();
    }
    if (type_ == Type::Boolean and op_ != '&' and op_ != '^' and op_ != '|') {
        _throw();
    }
    if (type_ == Type::Integer and op_ != '+' and op_ != '-' and op_ != '*' and op_ != '/' and op_ != '%') {
        _throw();
    }
}


database::value_t BinaryOperation::eval(int col) const {
    if (type_ == Type::Integer) {
        int v1 = get<int>(arg1_->eval(col));
        int v2 = get<int>(arg2_->eval(col));
        switch (op_) {
            case '+':
                return v1 + v2;
            case '-':
                return v2 - v1;
            case '*':
                return v2 * v1;
            case '/':
                if (v1 == 0) throw execution_error("Попытка деления на 0");
                return v2 / v1;
            case '%':
                if (v1 == 0) throw execution_error("Попытка деления на 0");
                return v2 % v1;
        }
    } else if (type_ == Type::Boolean) {
        bool v1 = get<bool>(arg1_->eval(col));
        bool v2 = get<bool>(arg2_->eval(col));
        switch (op_) {
            case '&':
                return static_cast<bool>(v1 && v2);
            case '^':
                return static_cast<bool>(v2 ^ v1);
            case '|':
                return static_cast<bool>(v2 || v1);
        }
    }
    return get<std::string>(arg1_->eval(col)) + get<std::string>(arg2_->eval(col));
}

void BinaryOperation::_throw() const {
    throw syntax_error(
            "Оператор `" + std::string(1 + ("%|^"s.contains(op_)), op_) + "` не определен для типов " +
            type_to_str(arg2_->type()) + ", " +
            type_to_str(arg1_->type())
    );
}

void UnaryOperation::bind(std::vector<std::unique_ptr<Operation>> &ops) {
    auto f = std::move(ops.back());
    ops.pop_back();
    f->bind(ops);

    arg_ = std::move(f);
    type_ = arg_->type();

    if (type_ == Type::Integer and op_ != '+' and op_ != '-') {
        _throw();
    }
    if (type_ == Type::Boolean and op_ != '!') {
        _throw();
    }
}


database::value_t UnaryOperation::eval(int col) const {
    if (type_ == Type::Integer) {
        int v = get<int>(arg_->eval(col));
        if (op_ == '-') {
            return -v;
        }
        return v;
    } else if (type_ == Type::Boolean) {
        return not get<bool>(arg_->eval(col));
    }
    _throw();
}

void UnaryOperation::_throw() const {
    throw syntax_error("Оператор `" + std::string(1, op_) + "` не определен для типа " + type_to_str(type_));
}

void ComparisonOperation::bind(std::vector<std::unique_ptr<Operation>> &ops) {
    auto f = std::move(ops.back());
    ops.pop_back();
    f->bind(ops);

    auto s = std::move(ops.back());
    ops.pop_back();
    s->bind(ops);

    arg1_ = std::move(f);
    arg2_ = std::move(s);
    type_ = Type::Boolean;

    if (arg1_->type() != arg2_->type()) {
        _throw();
    }
}


database::value_t ComparisonOperation::eval(int col) const {
    if (arg1_->type() == Type::Integer) {
        return _compare<int>(col);
    } else if (arg1_->type() == Type::String) {
        return _compare<std::string>(col);
    } else if (arg1_->type() == Type::Bytes) {
        return _compare<std::vector<bool>>(col);
    } else if (arg1_->type() == Type::Boolean) {
        return _compare<bool>(col);
    }
    _throw();
}

void ComparisonOperation::_throw() const {
    throw syntax_error("Оператор `" + op_ + "` не определен для типов " + type_to_str(arg2_->type()) + ", " +
                       type_to_str(arg1_->type())
    );
}

template<typename T>
bool ComparisonOperation::_compare(int col) const {
    T a = get<T>(arg2_->eval(col));
    T b = get<T>(arg1_->eval(col));
    if (op_ == ">") {
        return a > b;
    } else if (op_ == "<") {
        return a < b;
    } else if (op_ == "=") {
        return a == b;
    } else if (op_ == ">=") {
        return a >= b;
    } else if (op_ == "<=") {
        return a <= b;
    }
    _throw();
}

void LenOperation::_throw() const {
    throw syntax_error("Оператор `Len` не определен для типа " + type_to_str(arg_->type()));
}


void LenOperation::bind(std::vector<std::unique_ptr<Operation>> &ops) {
    auto f = std::move(ops.back());
    ops.pop_back();
    f->bind(ops);

    arg_ = std::move(f);
    type_ = Type::Integer;

    if (arg_->type() != Type::String and arg_->type() != Type::Bytes) {
        _throw();
    }
}

database::value_t LenOperation::eval(int col) const {
    if (arg_->type() == Type::String) {
        return static_cast<int>(get<std::string>(arg_->eval(col)).size());
    } else if (arg_->type() == Type::Bytes) {
        return static_cast<int>(get<std::vector<bool>>(arg_->eval(col)).size()) / 4;
    }
    _throw();
}

database::value_t FieldOperation::eval(int col) const {
    return col_->get_value(col);
}

void FieldOperation::resolve(ColumnContext &ctx) {
    if (!ctx.contains(name_)) {
        _throw();
    }
    col_ = ctx[name_];
}

void FieldOperation::_throw() const {
    throw syntax_error("Столбец `" + name_ + "` не найден");
}
