#include "gtest/gtest.h"
#include "create.h"

using namespace database;

class CreateSuite : public ::testing::Test {
public:
    std::map<std::string, Table> tables;
    TableContext ctx{tables, 0};
protected:
    void SetUp() override {
        tables.clear();
        ctx.number = 0;
    }

    void TearDown() override {
        tables.clear();
    }
};

TEST_F(CreateSuite, CreateSimple) {
    std::vector<std::shared_ptr<IColumn>> cols = {
            std::make_shared<Column<int>>(Type::Integer, "id", true, true, true, 0),
            std::make_shared<Column<std::string>>(Type::String, "id1", true, false, true,
                                                  "144 create\\\\n\\nall\\\\0asd\\n"),
    };
    auto res = Create().parse_and_execute(R"(Create table NAME
(
{key, autoincrement} id:integer=0,
{key, unique} id1:string = "144 create\n
all\0asd
"
)
)", ctx);
    ASSERT_EQ(res.columns().size(), 2);
    ASSERT_EQ(res.name(), "NAME");
    for (int i = 0; i < cols.size(); i++) {
        ASSERT_EQ(*res.columns()[i], *cols[i]);
    }
}


TEST_F(CreateSuite, CreateSimple2) {
    std::vector<bool> default_hash = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };
    std::vector<std::shared_ptr<IColumn>> cols = {
            std::make_shared<Column<int>>(Type::Integer, "id", true, true, true, 100),
            std::make_shared<Column<std::string>>(Type::String, "name", false, false, false, ""),
            std::make_shared<Column<std::string>>(Type::Bytes, "hash_id", true, false, true, default_hash),
            std::make_shared<Column<std::string>>(Type::Bool, "boolean_flag", false, false, false, false),
    };
    auto res = Create().parse_and_execute(R"(Create table table1(
{key, autoincrement} id:integer=100,
name:string,
{unique} hash_id:BYtES = 0x0000000000000fffffffffffff,
boolean_flag :      bOOl
)
)", ctx);
    ASSERT_EQ(res.columns().size(), 4);
    ASSERT_EQ(res.name(), "table1");
    for (int i = 0; i < cols.size(); i++) {
        ASSERT_EQ(*res.columns()[i], *cols[i]);
    }
}

TEST_F(CreateSuite, CreateSameError) {
    auto res = Create().parse_and_execute(R"(Create table table(col1:integer))", ctx);
    ASSERT_THROW(Create().parse_and_execute(R"(Create table table(col2:integer))", ctx), execution_error);
}


TEST_F(CreateSuite, CreationError) {
    std::string query1 = R"(Create table table
(
{key, autoincrement} id:integer=0,
{key, unique} id:string = ""))";
    std::string query2 = R"(Create table table(
{key, autoincrement} id:integer=0,
{key, unique, autoincrement} id1:string = ""))";
    std::string query3 = R"(Create table 123table(id:integer))";
    std::string query4 = R"(Create table table(id:integer = 7)";
    std::string query5 = R"(Create table table(id:string=5))";
    std::string query6 = R"(Create table table(id:bytes=0xabcg))";
    std::string query7 = R"(Create table table({autoincrement}id:bytes=5))";
    std::string query8 = R"(Create table table({{key}id:bytes=5))";
    std::string query9 = R"(Create table table({{key}}id:bytes=5))";
    std::string query10 = R"(Create table table({key unique}id:bytes))";
    std::string query11 = R"(Create table_or_not_table table(id:inteGer))";
    std::string query12 = R"(Create table named as table(id:inteGer))";
    ASSERT_THROW(Create().parse_and_execute(query1, ctx), execution_error);
    ASSERT_THROW(Create().parse_and_execute(query2, ctx), execution_error);
    ASSERT_THROW(Create().parse_and_execute(query3, ctx), syntax_error);
    ASSERT_THROW(Create().parse_and_execute(query4, ctx), syntax_error);
    ASSERT_THROW(Create().parse_and_execute(query5, ctx), syntax_error);
    ASSERT_THROW(Create().parse_and_execute(query6, ctx), syntax_error);
    ASSERT_THROW(Create().parse_and_execute(query7, ctx), execution_error);
    ASSERT_THROW(Create().parse_and_execute(query8, ctx), syntax_error);
    ASSERT_THROW(Create().parse_and_execute(query9, ctx), syntax_error);
    ASSERT_THROW(Create().parse_and_execute(query10, ctx), syntax_error);
    ASSERT_THROW(Create().parse_and_execute(query11, ctx), syntax_error);
    ASSERT_THROW(Create().parse_and_execute(query12, ctx), syntax_error);
    ASSERT_EQ(tables.size(), 0);
}
