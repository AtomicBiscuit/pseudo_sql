#include "gtest/gtest.h"

#define private public

#include "db.h"

using namespace database;

class DeleteSuite : public ::testing::Test {
public:
    DataBase db;
    DataBase db_copy;
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
        db_copy.load_from_file("../fixtures/fix1.bin");
    }
};

TEST_F(DeleteSuite, DeleteOne) {
    TableContext ctx = {db.tables_, 0};
    auto res = Delete().parse_and_execute("delete Users where id=190", ctx);
    auto copy = db_copy.tables_["Users"];
    ASSERT_EQ(res.columns()[0]->size(), 10000 - 1);
    int ind = 0;
    for (int i = 0; i < 10000; i++) {
        auto row = copy.row(i).row_;
        if (get<int>(row["id"]) == 190) {
            ind = -1;
            continue;
        }
        auto row_res = res.row(i + ind).row_;
        ASSERT_EQ(row, row_res);
    }
}

TEST_F(DeleteSuite, DeleteHalf) {
    TableContext ctx = {db.tables_, 0};
    auto res = Delete().parse_and_execute("delete Users where age < 75", ctx);
    auto copy = db_copy.tables_["Users"];
    int ind = 0;
    for (int i = 0; i < 10000; i++) {
        auto row = copy.row(i).row_;
        if (get<int>(row["age"]) < 75) {
            ind -= 1;
            continue;
        }
        auto row_res = res.row(i + ind).row_;
        ASSERT_EQ(row, row_res);
    }
    ASSERT_EQ(res.columns()[0]->size(), 10000 + ind);
}

TEST_F(DeleteSuite, DeleteAll) {
    TableContext ctx = {db.tables_, 0};
    auto res = Delete().parse_and_execute("delete Users where age < 75 ||age > 74", ctx);
    ASSERT_EQ(res.columns()[0]->size(), 0);
}

TEST_F(DeleteSuite, DeleteInsertConstrictSafety) {
    TableContext ctx = {db.tables_, 0};
    auto res = Delete().parse_and_execute("delete Users where id = 100", ctx);
    ASSERT_EQ(res.columns()[0]->size(), 10000 - 1);\
    auto name = get<std::string>(db_copy.tables_["Users"].row(0)["name"]);
    auto fname = get<std::string>(db_copy.tables_["Users"].row(0)["fname"]);
    res = Insert().parse_and_execute("insert (100, \"" + name + "\", true, 61, \"" + fname + "\" ) to Users", ctx);
    ASSERT_EQ(res.columns()[0]->size(), 10000);
    res = Select().parse_and_execute("select id, name, state, age, fname from Users where id=100", ctx);
    ASSERT_EQ(res.columns()[0]->size(), 1);
    ASSERT_EQ(res.row(0).row_, db_copy.tables_["Users"].row(0).row_);
}

TEST_F(DeleteSuite, DeleteZero) {
    TableContext ctx = {db.tables_, 0};
    auto res = Delete().parse_and_execute("delete Users where id = -|name|", ctx);
    ASSERT_EQ(res.columns()[0]->size(), 10000);\
    for (int i = 0; i < 10000; i++) {
        ASSERT_EQ(res.row(i).row_, db_copy.tables_["Users"].row(i).row_);
    }
}

TEST_F(DeleteSuite, DeletingError) {
    TableContext ctx = {db.tables_, 0};
    std::string query1 = "delete Users id = 7";
    std::string query2 = "delete Users where id=\"name\"";
    std::string query3 = "delete Users_Not_Exist where true";
    std::string query4 = "update Users where id>id-1";

    ASSERT_THROW(Delete().parse_and_execute(query1, ctx), syntax_error);
    ASSERT_THROW(Delete().parse_and_execute(query2, ctx), syntax_error);
    ASSERT_THROW(Delete().parse_and_execute(query3, ctx), execution_error);
    ASSERT_THROW(Delete().parse_and_execute(query4, ctx), syntax_error);
}