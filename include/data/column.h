#pragma once


#include "../syexception.h"
#include "serializer.h"

#include <optional>
#include <unordered_set>
#include <sstream>
#include <string>
#include <memory>
#include <map>
#include <algorithm>
#include <vector>
#include <variant>
#include <fstream>

namespace database {


    class IColumn {
    protected:
        Type type_;
        std::string name_;
        database::value_t default_value_;
        bool is_autoinc_ = false;
        bool is_default_ = false;
        bool is_unique_ = false;

    public:

        IColumn(Type type, const std::string &name) : type_(type), name_(name) {};

        IColumn(Type type, const std::string &name, bool is_unique, bool is_auto, bool is_def, const value_t &def)
                : type_(type), name_(name),
                  default_value_(def), is_autoinc_(is_auto), is_default_(is_def), is_unique_(is_unique) {
            if (is_autoinc_ and not holds_alternative<int>(default_value_)) {
                default_value_ = 0;
            }
        };

        IColumn(const IColumn &other) : type_(other.type_), name_(other.name_), is_autoinc_(other.is_autoinc_),
                                        is_default_(other.is_default_),
                                        is_unique_(other.is_unique_) {};

        IColumn(IColumn &&other) noexcept: type_(other.type_), name_(std::move(other.name_)),
                                           is_autoinc_(other.is_autoinc_), is_default_(other.is_default_),
                                           is_unique_(other.is_unique_) {};

        virtual ~IColumn() = default;

        virtual void check_valid() const = 0;

        Type type() const { return type_; }

        const std::string &name() const { return name_; }

        virtual std::shared_ptr<IColumn> multicopy(size_t, bool) const = 0;

        virtual void apply_update(const std::vector<int> &, std::vector<value_t> &) = 0;

        virtual void apply_delete(const std::vector<int> &) = 0;

        virtual std::shared_ptr<IColumn> copy_apply_delete(const std::vector<int> &, const std::string &) const = 0;

        virtual void add(std::optional<value_t> &&) = 0;

        virtual void save_to_file(std::ofstream &) const = 0;

        static std::shared_ptr<IColumn> load_from_file(std::ifstream &file);

        virtual size_t size() const = 0;

        virtual value_t get_value(int row) const = 0;

    private:
        virtual void _load_from_file_impl(std::ifstream &file) = 0;
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


        void set_rows(std::vector<T> &&rows) { rows_ = std::move(rows); }

        void check_valid() const override {
            EXEC_ASSERT(
                    !is_unique_ or std::unordered_set<value_t>(rows_.begin(), rows_.end()).size() == rows_.size(),
                    "Нарушено требование unique столбца " + name_);
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

        void apply_update(const std::vector<int> &rows, std::vector<value_t> &new_vals_) override {
            int ind = 0;
            for (auto i: rows) {
                rows_[i] = std::move(get<T>(new_vals_[ind++]));
            }
            if (is_unique_) {
                check_valid();
            }
        }

        void apply_delete(const std::vector<int> &rows) override {
            std::vector<T> res;
            res.reserve(rows.size());
            for (auto i: rows) {
                res.push_back(std::move(rows_[i]));
            }
            set_rows(std::move(res));
        }

        std::shared_ptr<IColumn>
        copy_apply_delete(const std::vector<int> &rows, const std::string &name) const override {
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


        void save_to_file(std::ofstream &file) const override {
            serialization::save_str(file, name_);
            serialization::save_int(file, static_cast<int>(type_));
            serialization::save_bool(file, is_autoinc_);
            serialization::save_bool(file, is_default_);
            serialization::save_bool(file, is_unique_);
            serialization::save<T>(file, get<T>(default_value_));
            serialization::save_int(file, rows_.size());
            for (const auto &val: rows_) {
                serialization::save<T>(file, val);
            }
        }

        virtual size_t size() const override { return rows_.size(); };

        value_t get_value(int row) const override { return rows_[row]; }

    private:
        void _load_from_file_impl(std::ifstream &file) override {
            is_autoinc_ = serialization::load_bool(file);
            is_default_ = serialization::load_bool(file);
            is_unique_ = serialization::load_bool(file);
            default_value_ = serialization::load<T>(file);
            int size = serialization::load_int(file);
            rows_.reserve(size);
            for (int i = 0; i < size; i++) {
                rows_.push_back(std::move(serialization::load<T>(file)));
            }
        }
    };


    using ColumnContext = std::map<std::string, std::shared_ptr<IColumn>>;
}