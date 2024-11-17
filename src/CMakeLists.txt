add_library(
        Database STATIC
        parser.cpp
        select.cpp
        expression.cpp
        token.cpp
        operations.cpp
        table.cpp
)