#include "../include/operations.h"
#include "../include/token.h"

using namespace database::operations;
using namespace std::string_literals;

PrioritizedOperation PrioritizedOperation::get_operation(std::string_view &view, OperationType last, bool is_len) {
    if ("+-!"s.contains(view[0]) and last != OperationType::Operand and last != OperationType::UnaryOperation) {
        char op = view[0];
        view.remove_prefix(1);
        return {std::make_unique<UnaryOperation>(op), OperationType::UnaryOperation, 5};
    } else if ("+-"s.contains(view[0]) and last == OperationType::Operand) {
        char op = view[0];
        view.remove_prefix(1);
        return {std::make_unique<BinaryOperation>(op), OperationType::BinaryOperation, 1};
    } else if ("*/%"s.contains(view[0]) and last == OperationType::Operand) {
        char op = view[0];
        view.remove_prefix(1);
        return {std::make_unique<BinaryOperation>(op), OperationType::BinaryOperation, 2};
    }

    if ((view.starts_with(">=") or view.starts_with("<=") or view.starts_with("!=")) and
        last == OperationType::Operand) {
        auto op = std::string(view.begin(), view.begin() + 2);
        view.remove_prefix(2);
        return {std::make_unique<ComparisonOperation>(op), OperationType::BinaryOperation, 0};
    } else if (">=<"s.contains(view[0]) and last == OperationType::Operand) {
        auto op = std::string(1, view[0]);
        view.remove_prefix(1);
        return {std::make_unique<ComparisonOperation>(op), OperationType::BinaryOperation, 0};
    }

    if (view[0] == '(' and last != OperationType::Operand) {
        view.remove_prefix(1);
        return {nullptr, OperationType::OpenScope, 10};
    } else if (view[0] == ')' and last == OperationType::Operand) {
        view.remove_prefix(1);
        return {nullptr, OperationType::CloseScope, 10};
    }

    if (view[0] == '|' and last != OperationType::Operand and !is_len) {
        view.remove_prefix(1);
        return {std::make_unique<LenOperation>(), OperationType::Len, 10};
    } else if (view[0] == '|' and is_len) {
        view.remove_prefix(1);
        return {nullptr, OperationType::Len, 10};
    } else if ((view.starts_with("&&") or view.starts_with("||") or view.starts_with("^^")) and
               last == OperationType::Operand) {
        char op = view[0];
        int prior = op == '&' ? -1 : -2;
        view.remove_prefix(2);
        return {std::make_unique<BinaryOperation>(op), OperationType::BinaryOperation, prior};
    }

    if (view[0] == '"' and last != OperationType::Operand) {
        return {std::make_unique<StringOperation>(tokenize::get_str(view)), OperationType::Operand, 20};
    } else if (view.starts_with("0x") and last != OperationType::Operand) {
        return {std::make_unique<BytesOperation>(tokenize::get_bytes(view)), OperationType::Operand, 20};
    } else if (isdigit(view[0]) and last != OperationType::Operand) {
        return {std::make_unique<IntegerOperation>(tokenize::get_int(view)), OperationType::Operand, 20};
    } else if (view.starts_with("false") and
               not(view.length() >= 6 and (isalnum(view[5]) or view[5] == '.' or view[5] == '_')) and
               last != OperationType::Operand) {
        view.remove_prefix(5);
        return {std::make_unique<BoolOperation>(false), OperationType::Operand, 20};
    } else if (view.starts_with("true") and
               not(view.length() >= 5 and (isalnum(view[4]) or view[4] == '.' or view[4] == '_')) and
               last != OperationType::Operand) {
        view.remove_prefix(4);
        return {std::make_unique<BoolOperation>(true), OperationType::Operand, 20};
    } else if (isalpha(view[0]) and last != OperationType::Operand) {
        return {std::make_unique<FieldOperation>(tokenize::get_full_name(view)), OperationType::Operand, 42};
    }

    SYNTAX_ASSERT(0, "Неразрешимый в контексте литерал: " + std::string(view));
}

PrioritizedOperation &PrioritizedOperation::operator=(PrioritizedOperation &&other) noexcept {
    oper_ = std::move(other.oper_);
    prior_ = other.prior_;
    type_ = other.type_;
    return *this;
}

PrioritizedOperation::PrioritizedOperation(std::unique_ptr<Operation> oper, OperationType type, int prior) :
        oper_(std::move(oper)), type_(type), prior_(prior) {}

PrioritizedOperation::PrioritizedOperation(PrioritizedOperation &&other) noexcept: oper_(std::move(other.oper_)),
                                                                                   type_(other.type_),
                                                                                   prior_(other.prior_) {}
