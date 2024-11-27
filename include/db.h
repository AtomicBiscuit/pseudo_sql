#pragma once

#include "command/select.h"
#include "command/create.h"
#include "command/insert.h"
#include "command/update.h"
#include "command/delete.h"

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

    void save_to_file(const std::string &) const;

    void load_from_file(const std::string &);

private:
    static std::shared_ptr<database::CommandExpression> _get_command(std::string_view &);
};