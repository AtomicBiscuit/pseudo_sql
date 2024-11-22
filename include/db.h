#pragma once

#include "select.h"
#include "create.h"
#include "insert.h"

#include <list>
#include <string>
#include <map>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

class DataBase {
private:
    std::map<std::string, std::shared_ptr<database::Table>> tables_;
    const static inline std::map<std::string, std::shared_ptr<database::CommandExpression>> commands_{
            {"select", std::make_shared<database::Select>()},
            {"create", std::make_shared<database::Create>()},
            {"insert", std::make_shared<database::Insert>()},
    };

    static std::shared_ptr<database::CommandExpression> _get_command(std::string_view &);

public:

    DataBase() = default;

    std::shared_ptr<database::Table> execute(const std::string &target);
};