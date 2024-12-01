#include "gtest/gtest.h"

#define private public

#include "db.h"

using namespace database;

class UpdateSuite : public ::testing::Test {
public:
    DataBase db;
protected:
    void SetUp() override {
        // Тестовы образ бд с таблицей Users из 10000 случайных записей с уникальными числами, именами, возрастом,
        // bool флагом, уникальными полными именами
        // create table Users(
        //	  {unique, key, autoincrement}id:InTeger=100,
        //	  name : string ,
        //	  state: bool,
        //    age:  integer,
        //    {unique} fname:string
        // );
        db.load_from_file("../fixtures/fix1.bin");
    }
};

TEST_F(UpdateSuite, UpdateAll) {
    TableContext ctx = {db.tables_, 0};
    auto res = Update().parse_and_execute(R"(
update Users set id=-id, age=|fname|-|name|, state = name>=fname where true
)", ctx);
    ASSERT_EQ(res.columns()[0]->size(), 10000);
    for (int i = 0; i < 10000; i++) {
        auto row = res.row(i).row_;
        ASSERT_EQ(get<bool>(row["state"]), get<std::string>(row["name"]) >= get<std::string>(row["fname"]));
        ASSERT_TRUE(get<int>(row["id"]) < 0);
        ASSERT_EQ(get<int>(row["age"]), get<std::string>(row["fname"]).size() - get<std::string>(row["name"]).size());
    }
}

TEST_F(UpdateSuite, UpdateHalf) {
    TableContext ctx = {db.tables_, 0};
    auto res = Update().parse_and_execute(R"(
update Users set age=-1 where state=false
)", ctx);
    ASSERT_EQ(res.columns()[0]->size(), 10000);
    for (int i = 0; i < 10000; i++) {
        auto row = res.row(i).row_;
        ASSERT_TRUE(get<bool>(row["state"]) or get<int>(row["age"]) == -1);
    }
}

TEST_F(UpdateSuite, UpdateOne) {
    TableContext ctx = {db.tables_, 0};
    auto res = Update().parse_and_execute(R"(
update Users set age=-1, id=0, name="Unique", fname="Dominic Torreto" where id = 7777
)", ctx);
    ASSERT_EQ(res.columns()[0]->size(), 10000);
    for (int i = 0; i < 10000; i++) {
        auto row = res.row(i).row_;
        if (get<int>(row["id"]) == 7777) {
            FAIL();
        } else if (get<int>(row["id"]) != 0) {
            continue;
        }
        ASSERT_EQ(get<int>(row["age"]), -1);
        ASSERT_EQ(get<std::string>(row["name"]), "Unique");
        ASSERT_EQ(get<std::string>(row["fname"]), "Dominic Torreto");
    }
}

TEST_F(UpdateSuite, UpdateCheckConstraintsOnlyAfterAll) {
    TableContext ctx = {db.tables_, 0};
    auto query = "update Users set id=id + 1 where true";
    ASSERT_NO_THROW(Update().parse_and_execute(query, ctx));
}

TEST_F(UpdateSuite, UpdateConstraintViolation) {
    TableContext ctx = {db.tables_, 0};
    auto query_int = "update Users set id=190 where id=191";
    ASSERT_THROW(Update().parse_and_execute(query_int, ctx), execution_error);
    auto query_str = "update Users set fname=\"Karen Bryan Lewis\" where fname=\"Larry Craig Scott\"";
    ASSERT_THROW(Update().parse_and_execute(query_str, ctx), execution_error);
}

TEST_F(UpdateSuite, UpdateZero) {
    TableContext ctx = {db.tables_, 0};
    auto query = "update Users set id=190 where id<100";
    ASSERT_NO_THROW(Update().parse_and_execute(query, ctx));
}

TEST_F(UpdateSuite, UpdatingError) {
    TableContext ctx = {db.tables_, 0};
    std::string query1 = "update users set id=160 where true"; // Название таблицы чувствительно к регистру
    std::string query2 = "update Users set where true";
    std::string query3 = "update Users set id=name where true";
    std::string query4 = "update Users set id=id where id % 2";
    std::string query5 = "update Users set id=id, id=-id where true";
    std::string query6 = "update Users set id=id where_is_where";
    std::string query7 = "update Users set id=id";
    std::string query8 = "update Users set id<=id where false";

    ASSERT_THROW(Update().parse_and_execute(query1, ctx), execution_error);
    ASSERT_THROW(Update().parse_and_execute(query2, ctx), syntax_error);
    ASSERT_THROW(Update().parse_and_execute(query3, ctx), execution_error);
    ASSERT_THROW(Update().parse_and_execute(query4, ctx), execution_error);
    ASSERT_THROW(Update().parse_and_execute(query5, ctx), execution_error);
    ASSERT_THROW(Update().parse_and_execute(query6, ctx), syntax_error);
    ASSERT_THROW(Update().parse_and_execute(query7, ctx), syntax_error);
    ASSERT_THROW(Update().parse_and_execute(query8, ctx), syntax_error);
}