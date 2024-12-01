#include "gtest/gtest.h"

#define private public

#include "db.h"

using namespace database;

class SelectSuit : public ::testing::Test {
public:
    DataBase db;
protected:
    void SetUp() override {
        db.load_from_file("../fixtures/fix1.bin");
    }
};

TEST_F(SelectSuit, SelectSimpleWithColumnsAndNames) {
    TableContext ctx{db.tables_, 0};
    auto res = ExecutionResult(Select().parse_and_execute(
            "SelecT id as not_id, age < 50 as is_old, name + \"_\" + fname as full_name from Users where state > false",
            ctx), true);
    ASSERT_TRUE(res.is_success());
    ASSERT_EQ(res.rows_number(), 5061);
    ASSERT_EQ(res.table_.name(), "Table1");
    ASSERT_EQ(res.columns_number(), 3);

    std::vector<std::map<std::string, value_t>> correct;
    for (int i = 0; i < 10000; i++) {
        if (get<bool>(db.tables_["Users"].row(i)["state"])) {
            correct.push_back(db.tables_["Users"].row(i).row_);
        }
    }
    int cnt = 0;
    for (auto row: res) {
        ASSERT_EQ(get<int>(row["not_id"]), get<int>(correct[cnt]["id"]));
        ASSERT_EQ(get<bool>(row["is_old"]), get<int>(correct[cnt]["age"]) < 50);
        ASSERT_EQ(get<std::string>(row["full_name"]),
                  get<std::string>(correct[cnt]["name"]) + "_" + get<std::string>(correct[cnt]["fname"]));
        cnt++;
    }
}

TEST_F(SelectSuit, SelectSimpleWithColumns) {
    TableContext ctx{db.tables_, 0};
    auto res = ExecutionResult(Select().parse_and_execute(
            "SelecT id, age > 50 as is_is, name + \"_\" + fname from Users where state <= false",
            ctx), true);
    ASSERT_TRUE(res.is_success());
    ASSERT_EQ(res.rows_number(), 4939);
    ASSERT_EQ(res.table_.name(), "Table1");
    ASSERT_EQ(res.columns_number(), 3);

    std::vector<std::map<std::string, value_t>> correct;
    for (int i = 0; i < 10000; i++) {
        if (not get<bool>(db.tables_["Users"].row(i)["state"])) {
            correct.push_back(db.tables_["Users"].row(i).row_);
        }
    }
    int cnt = 0;
    for (auto row: res) {
        ASSERT_EQ(get<int>(row["id"]),
                  get<int>(correct[cnt]["id"])); // Если выбрано подвыражение из только столбца, то его имя сохраняется
        ASSERT_EQ(get<bool>(row["is_is"]), get<int>(correct[cnt]["age"]) > 50); // Имя задано явно
        ASSERT_EQ(get<std::string>(
                row["column3"]), // Если используется выражение со столбцами и имя не указано, ему присваивается columni - где i номер столбца в результирующей матрице
                  get<std::string>(correct[cnt]["name"]) + "_" + get<std::string>(correct[cnt]["fname"]));
        cnt++;
    }
}

TEST_F(SelectSuit, SelectJoinWithSubQuery) {
    TableContext ctx{db.tables_, 0};
    auto res = ExecutionResult(Select().parse_and_execute(
            R"(SelecT U.id as UID
            from (select id, age from Users where id%101=5) as U
                join (select age, id from Users where id%101=7) as u on U.id<u.id
            where U.age >= u.age)",
            ctx), true);
    ASSERT_TRUE(res.is_success());
    ASSERT_EQ(res.columns_number(), 1);
    ASSERT_EQ(res.columns()[0].first, "UID");
    ASSERT_EQ(res.table_.name(), "Table3");

    std::vector<int> correct;
    for (int i = 0; i < 10000; i++) {
        if (get<int>(db.tables_["Users"].row(i)["id"]) % 101 != 7) {
            continue;
        }
        auto uage = get<int>(db.tables_["Users"].row(i)["age"]);
        for (int j = 0; j < i; j++) {
            if (get<int>(db.tables_["Users"].row(j)["id"]) % 101 != 5) {
                continue;
            }
            auto Uage = get<int>(db.tables_["Users"].row(j)["age"]);
            if (Uage >= uage) {
                correct.push_back(get<int>(db.tables_["Users"].row(j)["id"]));
            }
        }
    }

    ASSERT_EQ(res.rows_number(), correct.size()); // 2547
    for (int i = 0; i < correct.size(); i++) {
        ASSERT_EQ(get<int>(res.table_.row(i)["UID"]), correct[i]);
    }
}

TEST_F(SelectSuit, SelectJoinWithBadNaming) {
    TableContext ctx{db.tables_, 0};

    // Пока у таблиц с одинаковым именем нет одинаковых столбцов, всё в порядке
    auto res = ExecutionResult(Select().parse_and_execute(
            "select Users.id + Users.not_id "
            "from (select id as not_id from Users where id=100) as Users "
            "join Users on true where true",
            ctx), true);
    ASSERT_TRUE(res.is_success());
    ASSERT_EQ(res.columns_number(), 1);
    ASSERT_EQ(res.columns()[0].first, "column1");
    ASSERT_EQ(res.table_.name(), "Table2");

    ASSERT_EQ(res.rows_number(), 10000);
    for (int i = 0; i < 10000; i++) {
        ASSERT_EQ(get<int>(res.table_.row(i)["column1"]), get<int>(db.tables_["Users"].row(i)["id"]) + 100);
    }
}

TEST_F(SelectSuit, SelectingError) {
    TableContext ctx = {db.tables_, 0};
    std::string query1 = "select idx from Users where true";
    std::string query2 = "select id frot Users where true";
    std::string query3 = "select id from Users on true";
    std::string query4 = "select id from Users join Users on true where true"; // Одинаковые имена столбцов Users.id
    std::string query5 = "select Users.id+\"c\" from (select 1 from Users where id=100) as U join Users on false where true";
    std::string query6 = "select Users.id as Users.not_id from Users where true";
    std::string query7 = "select U.id + U.not_id from (select id as not_id, id from Users where id=100) as U join Users as U on true where true";

    ASSERT_THROW(Select().parse_and_execute(query1, ctx), execution_error);
    ASSERT_THROW(Select().parse_and_execute(query2, ctx), syntax_error);
    ASSERT_THROW(Select().parse_and_execute(query3, ctx), syntax_error);
    ASSERT_THROW(Select().parse_and_execute(query4, ctx), execution_error);
    ASSERT_THROW(Select().parse_and_execute(query5, ctx), syntax_error);
    ASSERT_THROW(Select().parse_and_execute(query6, ctx), syntax_error);
    ASSERT_THROW(Select().parse_and_execute(query7, ctx), execution_error);
}