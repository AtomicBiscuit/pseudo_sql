add_library(
        Database STATIC
        db.cpp
        select.cpp
        expression.cpp
        token.cpp
        operations.cpp
        table.cpp
        create.cpp
        prior_oper.cpp
)