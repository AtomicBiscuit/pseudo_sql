#pragma once

#include "syexception.h"

#include <variant>
#include <vector>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_set>
#include <optional>

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
    protected:
        Type type_;
        std::string name_;
        value_t default_value_;
        bool is_default_ = false;
        bool is_unique_ = false;
        bool is_autoinc_ = false;
    public:

        IColumn(Type type, const std::string &name) : type_(type), name_(name) {};

        IColumn(Type type, const std::string &name, bool is_unique, bool is_auto, bool is_def, const value_t &def)
                : type_(type), name_(name),
                  default_value_(def), is_default_(is_def), is_unique_(is_unique), is_autoinc_(is_auto) {
            if (is_autoinc_ and not holds_alternative<int>(default_value_)) {
                default_value_ = 0;
            }
        };

        IColumn(const IColumn &other) : type_(other.type_), name_(other.name_), is_default_(other.is_default_),
                                        is_unique_(other.is_unique_), is_autoinc_(other.is_autoinc_) {};

        IColumn(IColumn &&other) noexcept: type_(other.type_), name_(std::move(other.name_)),
                                           is_default_(other.is_default_), is_unique_(other.is_unique_),
                                           is_autoinc_(other.is_autoinc_) {};

        virtual ~IColumn() = default;

        virtual void check_valid() const = 0;

        Type type() const { return type_; }

        const std::string &name() const { return name_; }

        virtual std::shared_ptr<IColumn> multicopy(size_t, bool) const = 0;

        virtual void apply_changes(const std::vector<int> &) = 0;

        virtual std::shared_ptr<IColumn> copy_apply_changes(const std::vector<int> &, const std::string &) const = 0;

        virtual void add(std::optional<value_t> &&) = 0;

        virtual size_t size() const = 0;

        virtual value_t get_value(int row) const = 0;
    };

    template<typename T> requires(std::convertible_to<T, value_t>)
    class Column : public IColumn {
        std::vector<T> rows_;
    public:
        Column(Type type, const std::string &name) : IColumn(type, name) {};

        Column(Type type, const std::string &name, bool is_unique, bool is_auto, bool is_def, const value_t &def)
                : IColumn(type, name, is_unique, is_auto, is_def, def) {};

        Column(const Column &other) : IColumn(other), rows_(other.rows_) {};

        Column(Column &&other) noexcept: IColumn(std::move(other)), rows_(std::move(other.rows_)) {};


        void set_rows(std::vector<T> &&rows) {
            rows_ = std::move(rows);
        }

        void check_valid() const override {
            if (is_unique_ and std::unordered_set<value_t>(rows_.begin(), rows_.end()).size() != rows_.size()) {
                throw execution_error("Нарушено требование unique столбца " + name_);
            }
        }

        std::shared_ptr<IColumn> multicopy(size_t cnt, bool is_seq) const override {
            auto res = std::make_shared<Column<T>>(type_, name_);
            std::vector<T> rows;
            rows.reserve(rows.size() * cnt);
            if (is_seq) {
                for (size_t i = 0; i < cnt; i++) {
                    rows.insert(rows.end(), rows_.begin(), rows_.end());
                }
            } else {
                for (const auto &i: rows_) {
                    rows.insert(rows.end(), cnt, i);
                }
            }
            res->set_rows(std::move(rows));
            return std::move(res);
        }

        void apply_changes(const std::vector<int> &rows) override {
            std::vector<T> res;
            res.reserve(rows.size());
            for (auto i: rows) {
                res.push_back(std::move(rows_[i]));
            }
            set_rows(std::move(res));
        }

        std::shared_ptr<IColumn>
        copy_apply_changes(const std::vector<int> &rows, const std::string &name) const override {
            std::vector<T> res;
            res.reserve(rows.size());
            for (auto i: rows) {
                res.push_back(rows_[i]);
            }
            auto temp = std::make_shared<Column<T>>(type_, name);
            temp->set_rows(std::move(res));
            return temp;
        }

        void add(std::optional<value_t> &&val) override {
            value_t new_val;
            if (!val.has_value()) {
                EXEC_ASSERT(is_default_ or is_autoinc_, "Не указано значение для столбца: " + name_);
                new_val = default_value_;
                if (is_autoinc_) {
                    ++get<int>(default_value_);
                }
            } else {
                new_val = std::move(val.value());
            }
            EXEC_ASSERT(not is_unique_ or std::find(rows_.begin(), rows_.end(), get<T>(new_val)) == rows_.end(),
                        "Добавление значения `" + value_to_string(new_val, type_) + "` в столбец " + name_ +
                        " нарушает требование уникальности");
            rows_.push_back(std::move(get<T>(new_val)));
        }

        virtual size_t size() const override { return rows_.size(); };

        value_t get_value(int row) const override { return rows_[row]; }
    };

    class Table {
        std::string name_;
        std::vector<std::shared_ptr<IColumn>> cols_;
    public:
        Table() = default;

        explicit Table(const std::string &name) : name_(name) {};

        std::vector<std::shared_ptr<IColumn>> get_columns() { return cols_; }

        std::string name() { return name_; }

        void add_column(const std::shared_ptr<IColumn> &col) { cols_.push_back(col); }

        void add_row(std::vector<std::optional<value_t>> &&row) {
            int i = 0;
            for (auto &col: cols_) {
                col->add(std::move(row[i++]));
            }
        }

        void check_valid() const;

        std::shared_ptr<Table> copy() const;
    };


    using ColumnContext = std::map<std::string, std::shared_ptr<IColumn>>;

    struct TableContext {
        std::map<std::string, std::shared_ptr<database::Table>> &tables;
        int number;
    };
}