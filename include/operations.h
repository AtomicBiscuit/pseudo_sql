#pragma once

#include "table.h"

#include <map>
#include <vector>
#include <string>
#include <memory>

namespace database {
    class Operation {
        virtual void _throw() const = 0;

    protected:
        Type type_ = Type::None;
    public:
        virtual value_t eval(int col) const = 0;

        virtual void bind(std::vector<std::unique_ptr<Operation>> &ops) = 0;

        Type type() const { return type_; }

        virtual ~Operation() = default;
    };

    class BinaryOperation : public Operation {
        char op_;
        std::unique_ptr<Operation> arg1_, arg2_;

        void _throw() const override;

    public:
        explicit BinaryOperation(char op) : op_(op) {};

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override;

        value_t eval(int col) const override;

    };

    class ComparisonOperation : public Operation {
        std::string op_;
        std::unique_ptr<Operation> arg1_, arg2_;

        void _throw() const override;

        template<typename T>
        bool _compare(int col) const;

    public:
        explicit ComparisonOperation(const std::string &op) : op_(op) {};

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override;

        value_t eval(int col) const override;

    };

    class UnaryOperation : public Operation {
        char op_;
        std::unique_ptr<Operation> arg_;

        void _throw() const override;

    public:
        explicit UnaryOperation(char op) : op_(op) {};

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override;

        value_t eval(int col) const override;
    };

    class LenOperation : public Operation {
        std::unique_ptr<Operation> arg_;

        void _throw() const override;

    public:
        LenOperation() = default;

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override;

        value_t eval(int col) const override;
    };

    class IntegerOperation : public Operation {
        int val_;

        void _throw() const override {};
    public:
        explicit IntegerOperation(int val) : val_(val) { type_ = Type::Integer; };

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        value_t eval(int col) const override { return val_; };
    };

    class StringOperation : public Operation {
        std::string val_;

        void _throw() const override {};
    public:
        explicit StringOperation(const std::string &val) : val_(val) { type_ = Type::String; };

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        value_t eval(int col) const override { return val_; };
    };

    class BoolOperation : public Operation {
        bool val_;

        void _throw() const override {};
    public:
        explicit BoolOperation(bool val) : val_(val) { type_ = Type::Boolean; };

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        value_t eval(int col) const override { return val_; };
    };

    class BytesOperation : public Operation {
        std::vector<bool> val_;

        void _throw() const override {};
    public:
        explicit BytesOperation(const std::vector<bool> &val) : val_(val) { type_ = Type::Bytes; };

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        value_t eval(int col) const override { return val_; };
    };

    class FieldOperation : public Operation {
        std::string name_;
        std::shared_ptr<IColumn> col_;

        void _throw() const override;

    public:
        explicit FieldOperation(const std::string &val) : name_(val) {};

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        void resolve(ColumnContext &);

        const std::string &name() const { return name_; };

        value_t eval(int col) const override;
    };
}