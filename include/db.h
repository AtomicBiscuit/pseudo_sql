#pragma once

#include "select.h"
#include "create.h"

#include <list>
#include <string>
#include <map>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

class DataBase {
private:
    database::TableContext tables_;
    const static inline std::map<std::string, std::shared_ptr<database::CommandExpression>> commands_{
            {"select", std::make_shared<database::Select>()},
            {"create", std::make_shared<database::Create>()}
    };

    static std::shared_ptr<database::CommandExpression> _get_command(std::stringstream &);

public:

    DataBase() = default;

    std::shared_ptr<database::Table> execute(const std::string &target);
};