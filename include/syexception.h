#pragma once

#include <stdexcept>

class syntax_error : std::exception {
private:
    std::string what_;
public:
    explicit syntax_error(const std::string &arg) : what_(arg) {};

    syntax_error(const syntax_error &other) : what_(other.what_) {};

    syntax_error(syntax_error &&other) noexcept: what_(std::move(other.what_)) {};

    const char *what() const noexcept override {
        return what_.c_str();
    }
};

class execution_error : std::exception {
private:
    std::string what_;
public:
    explicit execution_error(const std::string &arg) : what_(arg) {};

    execution_error(const execution_error &other) : what_(other.what_) {};

    execution_error(execution_error &&other) noexcept: what_(std::move(other.what_)) {};

    const char *what() const noexcept override {
        return what_.c_str();
    }
};