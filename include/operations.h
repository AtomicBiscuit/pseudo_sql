#pragma once

#include "data/table.h"
#include "syexception.h"
#include "data/column.h"

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <ranges>
#include <algorithm>
#include <list>
#include <utility>
#include <iostream>
#include <tuple>
#include <stack>

namespace database::operations {
    class Operation {
    protected:
        Type type_ = Type::None;
    public:
        virtual value_t eval(int col) const = 0;

        virtual void bind(std::vector<std::unique_ptr<Operation>> &ops) = 0;

        virtual void resolve(ColumnContext &) = 0;

        Type type() const { return type_; }

        virtual ~Operation() = default;

    private:
        virtual void _throw() const = 0;
    };

    class BinaryOperation : public Operation {
        char op_;
        std::unique_ptr<Operation> arg1_, arg2_;
    public:
        explicit BinaryOperation(char op) : op_(op) {};

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override;

        void resolve(ColumnContext &) override;

        value_t eval(int col) const override;

    private:
        void _throw() const override;
    };

    class ComparisonOperation : public Operation {
        std::string op_;
        std::unique_ptr<Operation> arg1_, arg2_;
    public:
        explicit ComparisonOperation(const std::string &op) : op_(op) {};

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override;

        void resolve(ColumnContext &) override;

        value_t eval(int col) const override;

    private:
        template<typename T>
        bool _compare(int col) const;

        void _throw() const override;
    };

    class UnaryOperation : public Operation {
        char op_;
        std::unique_ptr<Operation> arg_;
    public:
        explicit UnaryOperation(char op) : op_(op) {};

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override;

        void resolve(ColumnContext &) override;

        value_t eval(int col) const override;

    private:
        void _throw() const override;
    };

    class LenOperation : public Operation {
        std::unique_ptr<Operation> arg_;
    public:
        LenOperation() = default;

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override;

        void resolve(ColumnContext &) override;

        value_t eval(int col) const override;

    private:
        void _throw() const override;
    };

    class IntegerOperation : public Operation {
        int val_;
    public:
        explicit IntegerOperation(int val) : val_(val) { type_ = Type::Integer; };

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        void resolve(ColumnContext &) override {};

        value_t eval(int col) const override { return val_; };
    private:
        void _throw() const override {};
    };

    class StringOperation : public Operation {
        std::string val_;
    public:
        explicit StringOperation(const std::string &val) : val_(val) { type_ = Type::String; };

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        void resolve(ColumnContext &) override {};

        value_t eval(int col) const override { return val_; };
    private:
        void _throw() const override {};
    };

    class BoolOperation : public Operation {
        bool val_;
    public:
        explicit BoolOperation(bool val) : val_(val) { type_ = Type::Bool; };

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        void resolve(ColumnContext &) override {};

        value_t eval(int col) const override { return val_; };
    private:
        void _throw() const override {};
    };

    class BytesOperation : public Operation {
        std::vector<bool> val_;
    public:
        explicit BytesOperation(const std::vector<bool> &val) : val_(val) { type_ = Type::Bytes; };

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        void resolve(ColumnContext &) override {};

        value_t eval(int col) const override { return val_; };
    private:
        void _throw() const override {};
    };

    class FieldOperation : public Operation {
        std::string name_;
        std::shared_ptr<IColumn> col_;
    public:
        explicit FieldOperation(const std::string &val) : name_(val) {};

        void bind(std::vector<std::unique_ptr<Operation>> &ops) override {};

        void resolve(ColumnContext &) override;

        const std::string &name() const { return name_; };

        value_t eval(int row) const override;

    private:
        void _throw() const override;
    };

    enum class OperationType {
        Unary, Binary,
        Operand,
        OpenScope, CloseScope,
        Len
    };

    struct PrioritizedOperation {
        std::unique_ptr<Operation> oper_;
        OperationType type_;
        int prior_;

        PrioritizedOperation(std::unique_ptr<Operation> oper, OperationType type, int prior);

        PrioritizedOperation(PrioritizedOperation &&other) noexcept;

        PrioritizedOperation &operator=(PrioritizedOperation &&other) noexcept;

        static PrioritizedOperation get_operation(std::string_view &view, OperationType last, bool is_len);
    };
}