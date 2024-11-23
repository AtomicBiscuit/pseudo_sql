#pragma once

#include "select.h"
#include "create.h"
#include "insert.h"
#include "update.h"
#include "delete.h"

#include <list>
#include <string>
#include <map>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

class DataBase {
private:
    std::map<std::string, database::Table> tables_;
    const static inline std::map<std::string, std::shared_ptr<database::CommandExpression>> commands_{
            {"select", std::make_shared<database::Select>()},
            {"create", std::make_shared<database::Create>()},
            {"insert", std::make_shared<database::Insert>()},
            {"update", std::make_shared<database::Update>()},
            {"delete", std::make_shared<database::Delete>()},
    };

public:

    DataBase() = default;

    database::Table execute(const std::string &target);

private:
    static std::shared_ptr<database::CommandExpression> _get_command(std::string_view &);
};