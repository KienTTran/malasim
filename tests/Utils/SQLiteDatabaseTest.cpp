#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "Utils/Helpers/SQLiteDatabase.h"

TEST(SQLiteDatabaseTest, BindsSupportedTypesAndReturnsInsertedRowId) {
  SQLiteDatabase database(":memory:");
  database.execute("CREATE TABLE values_table (id INTEGER PRIMARY KEY, i INT, u16 INT, s16 INT, "
                   "u32 INT, timestamp INT, real_value REAL, text_value TEXT, c_text TEXT);");

  const auto id = database.insert_data(
      "INSERT INTO values_table (i, u16, s16, u32, timestamp, real_value, text_value, c_text) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?) RETURNING id;",
      7, static_cast<std::uint16_t>(8), static_cast<std::int16_t>(-9),
      static_cast<std::uint32_t>(10), static_cast<std::time_t>(11), 1.5, std::string("text"),
      "c-text");

  EXPECT_EQ(id, 1);
}

TEST(SQLiteDatabaseTest, HandlesTransactionsAndUnsupportedBinding) {
  SQLiteDatabase database(":memory:");
  database.execute("CREATE TABLE values_table (id INTEGER PRIMARY KEY, value INT);");
  {
    TransactionGuard transaction(&database);
    database.execute("INSERT INTO values_table (value) VALUES (1);");
    transaction.commit();
    transaction.commit();
  }

  EXPECT_NO_THROW(database.begin_transaction());
  EXPECT_NO_THROW(database.commit_transaction());
  EXPECT_THROW(database.insert_data("INSERT INTO values_table (value) VALUES (?) RETURNING id;",
                                    std::vector<int>{1}),
               std::runtime_error);
}

TEST(SQLiteDatabaseTest, LogsSqlErrorsAndPrepareFailuresWithoutCrashing) {
  SQLiteDatabase database(":memory:");
  EXPECT_NO_THROW(database.execute("not valid sql"));
  EXPECT_EQ(database.prepare("not valid sql"), nullptr);
}
