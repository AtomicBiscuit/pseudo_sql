add_library(
        Database STATIC
        db.cpp
        command/select.cpp
        expression.cpp
        token.cpp
        operations.cpp
        data/table.cpp
        command/create.cpp
        prior_oper.cpp
        command/insert.cpp
        command/update.cpp
        command/delete.cpp
        data/serializer.cpp
        data/column.cpp
)