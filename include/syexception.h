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