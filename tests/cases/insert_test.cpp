#include "gtest/gtest.h"

#define private public

#include "db.h"

using namespace database;

class InsertSuite : public ::testing::Test {
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

TEST_F(InsertSuite, InsertSimple) {
    TableContext ctx = {db.tables_, 0};
    std::map<std::string, value_t> correct = {
            {"id",    10100},
            {"name",  "Petr"},
            {"age",   42},
            {"state", false},
            {"fname", "Petr Ivanovich P"}
    };
    auto res = Insert().parse_and_execute(R"(Insert (, "Petr",  false, 42, "Petr Ivanovich P") to Users)", ctx);
    auto result = res.row(10000);
    ASSERT_EQ(result.row_, correct);
}

TEST_F(InsertSuite, InsertByNames) {
    TableContext ctx = {db.tables_, 0};
    std::map<std::string, value_t> correct = {
            {"id",    10100},
            {"name",  "Petr"},
            {"age",   42},
            {"state", false},
            {"fname", "Petr Ivanovich P"}
    };
    auto res = Insert().parse_and_execute(
            R"(Insert (fname="Petr Ivanovich P", state=false, name="Petr",  age=42) to Users)", ctx);
    auto result = res.row(10000);
    ASSERT_EQ(result.row_, correct);
}


TEST_F(InsertSuite, InsertionError) {
    TableContext ctx = {db.tables_, 0};
    std::string query1 = R"(Insert to Users (,,,,))";
    std::string query2 = R"(insert (100, "f", false, 42, "fname") to Users)";
    std::string query3 = R"(insert (, "f", false, 42, "Andrew Frank Lewis") to Users)";
    std::string query4 = R"(insert (, "f", 42, true, "unique") to Users)";
    std::string query5 = R"(insert (, 0x42, false, 0, "not a name") to Users)";
    std::string query6 = R"(insert (name="F", state="false", age=0, fname="not a name") to Users)";
    std::string query7 = R"(insert (name="F", name="false", state=true, age=0, fname="not a name") to Users)";
    std::string query8 = R"(insert (state=true, age=0, fname="not a name") to Users)";
    std::string query9 = R"(insert (, name="F", state=true, age=0, fname="not a name") to Users)";

    ASSERT_THROW(Insert().parse_and_execute(query1, ctx), syntax_error);
    ASSERT_THROW(Insert().parse_and_execute(query2, ctx), execution_error);
    ASSERT_THROW(Insert().parse_and_execute(query3, ctx), execution_error);
    ASSERT_THROW(Insert().parse_and_execute(query4, ctx), syntax_error);
    ASSERT_THROW(Insert().parse_and_execute(query5, ctx), syntax_error);
    ASSERT_THROW(Insert().parse_and_execute(query6, ctx), syntax_error);
    ASSERT_THROW(Insert().parse_and_execute(query7, ctx), syntax_error);
    ASSERT_THROW(Insert().parse_and_execute(query8, ctx), execution_error);
    ASSERT_THROW(Insert().parse_and_execute(query9, ctx), syntax_error);
    ASSERT_EQ(db.tables_["Users"].columns()[0]->size(), 10000);
}
