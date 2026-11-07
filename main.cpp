#include <iostream>
#include <stdexcept>
#include <string>
#include <optional>
#include <vector>
#include <filesystem>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>

#include "Operator.h"
#include "CallRecord.h"
#include "Database.h"
#include "IOperatorRepository.h"
#include "SQLiteOperatorRepository.h"
#include "TestData.h"

// Виконання операцій із базою даних через SQLite C API

int insertOperator(sqlite3* db, const Operator& op)
{
    const char* sql = R"(
        INSERT INTO Operator (fullName, department, status)
        VALUES (?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    rc = sqlite3_bind_text(stmt, 1, op.fullName.c_str(), -1, SQLITE_TRANSIENT);
    if (rc == SQLITE_OK)
    {
        rc = sqlite3_bind_text(stmt, 2, op.department.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (rc == SQLITE_OK)
    {
        rc = sqlite3_bind_text(stmt, 3, op.status.c_str(), -1, SQLITE_TRANSIENT);
    }

    if (rc != SQLITE_OK)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error(error);
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error(error);
    }

    int id = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);
    return id;
}

std::optional<Operator> findOperatorById(sqlite3* db, int id)
{
    const char* sql = R"(
        SELECT id, fullName, department, status
        FROM Operator
        WHERE id = ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    rc = sqlite3_bind_int(stmt, 1, id);
    if (rc != SQLITE_OK)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error(error);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        Operator op;
        op.id = sqlite3_column_int(stmt, 0);
        op.fullName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.department = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        op.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        sqlite3_finalize(stmt);
        return op;
    }

    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

void runRawSQLiteDemo()
{
    std::cout << "CallCenter ORM project\n";
    std::cout << "SQLite version: " << sqlite3_libversion() << '\n';

    sqlite3* db = nullptr;
    int rc = sqlite3_open("callcenter.db", &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return;
    }

    std::cout << "Database opened successfully\n";

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS Operator (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            fullName TEXT NOT NULL,
            department TEXT NOT NULL,
            status TEXT NOT NULL
        );
    )";

    char* errorMessage = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errorMessage);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: "
            << (errorMessage ? errorMessage : "Unknown error") << '\n';
        sqlite3_free(errorMessage);
        sqlite3_close(db);
        return;
    }

    std::cout << "Operator table is ready\n";

    try
    {
        Operator op{ 0, "Serhii Melnyk", "Technical Support", "online" };
        int id = insertOperator(db, op);
        std::cout << "Inserted operator ID: " << id << '\n';

        auto result = findOperatorById(db, id);
        if (result)
        {
            std::cout << "Operator found:\n";
            std::cout << "ID: " << result->id << '\n';
            std::cout << "FullName: " << result->fullName << '\n';
            std::cout << "Department: " << result->department << '\n';
            std::cout << "Status: " << result->status << '\n';
        }
        else
        {
            std::cout << "Operator not found\n";
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Database error: " << ex.what() << '\n';
    }

    sqlite3_close(db);
}

// Створення ORM-відображення та виконання операцій через sqlite_orm

void runOrmDemo()
{
    auto storage = createStorage("callcenter.db");
    storage.sync_schema();

    Operator op{ 0, "Nataliia Boyko", "Sales", "online" };
    int id = storage.insert(op);

    std::cout << "\nORM demonstration\n";
    std::cout << "Inserted operator ID: " << id << '\n';

    auto result = storage.get_optional<Operator>(id);
    if (result)
    {
        std::cout << "Operator found:\n";
        std::cout << "ID: " << result->id << '\n';
        std::cout << "FullName: " << result->fullName << '\n';
        std::cout << "Department: " << result->department << '\n';
        std::cout << "Status: " << result->status << '\n';
    }
    else
    {
        std::cout << "Operator not found\n";
    }
}

// Створення пов'язаної сутності CallRecord 


void runRelationshipDemo()
{
    auto storage = createStorage("callcenter.db");
    storage.open_forever();
    storage.on_open = [](sqlite3* db)
        {
            sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
        };
    storage.sync_schema();

    Operator op{ 0, "Viktor Savchenko", "Billing", "busy" };
    int operatorId = storage.insert(op);

    std::cout << "\nRelationship demonstration\n";
    std::cout << "Operator ID: " << operatorId << '\n';

    CallRecord c1{ 0, operatorId, "2026-10-10", "Balance inquiry", 180.0 };
    CallRecord c2{ 0, operatorId, "2026-10-20", "Payment confirmation", 340.0 };

    int callId1 = storage.insert(c1);
    int callId2 = storage.insert(c2);

    std::cout << "CallRecord records inserted: " << callId1 << ", " << callId2 << '\n';

    using namespace sqlite_orm;
    auto callRecords = storage.get_all<CallRecord>(
        where(c(&CallRecord::operatorId) == operatorId)
    );

    std::cout << "\nCall history:\n";
    for (const auto& record : callRecords)
    {
        std::cout << "ID: " << record.id
            << ", Date: " << record.callDate
            << ", Topic: " << record.topic
            << ", Duration: " << record.durationSeconds << '\n';
    }

    try
    {
        CallRecord invalidRecord{ 0, -1, "2026-10-25", "Invalid call", 100.0 };
        storage.insert(invalidRecord);
        std::cout << "Unexpected insertion success\n";
    }
    catch (const std::exception& ex)
    {
        std::cout << "\nForeign key constraint test:\n";
        std::cout << "Insertion rejected: " << ex.what() << '\n';
    }
}


// Реалізація патерна Repository

void runRepositoryDemo()
{
    SQLiteOperatorRepository sqliteRepository("callcenter.db");
    IOperatorRepository& repository = sqliteRepository;

    std::cout << "\nRepository demonstration\n";

    // CREATE
    Operator op{ 0, "Kateryna Moroz", "Technical Support", "online" };
    int id = repository.add(op);
    std::cout << "\nCREATE\n";
    std::cout << "Inserted operator ID: " << id << '\n';

    // READ
    std::cout << "\nREAD\n";
    auto found = repository.findById(id);
    if (found)
    {
        std::cout << "ID: " << found->id << '\n';
        std::cout << "FullName: " << found->fullName << '\n';
        std::cout << "Department: " << found->department << '\n';
        std::cout << "Status: " << found->status << '\n';
    }

    // UPDATE
    std::cout << "\nUPDATE\n";
    if (found)
    {
        found->status = "busy";
        bool updated = repository.update(*found);
        std::cout << "Updated: " << std::boolalpha << updated << '\n';

        auto afterUpdate = repository.findById(id);
        if (afterUpdate)
        {
            std::cout << "New status: " << afterUpdate->status << '\n';
        }
    }

    // READ ALL
    std::cout << "\nREAD ALL\n";
    auto allOperators = repository.findAll();
    for (const auto& item : allOperators)
    {
        std::cout << item.id << " | "
            << item.fullName << " | "
            << item.department << " | "
            << item.status << '\n';
    }

    // DELETE
    std::cout << "\nDELETE\n";
    bool removed = repository.remove(id);
    std::cout << "Removed: " << std::boolalpha << removed << '\n';

    // Перевірка після видалення
    auto afterDelete = repository.findById(id);
    std::cout << "Record exists after deletion: "
        << std::boolalpha << afterDelete.has_value() << '\n';
}

// Виконання запитів через ORM

void runQueryDemo()
{
    using namespace sqlite_orm;
    auto storage = createStorage("callcenter_queries.db");
    storage.sync_schema();

    std::cout << "\nORM QUERY DEMONSTRATION\n";

    //  Фільтрація за статусом
    std::cout << "\n1. Online operators:\n";
    auto onlineOperators = storage.get_all<Operator>(
        where(c(&Operator::status) == "online")
    );
    for (const auto& item : onlineOperators)
    {
        std::cout << item.id << " | "
            << item.fullName << " | "
            << item.status << '\n';
    }

    // Сортування за ПІБ
    std::cout << "\n2. Operators sorted by fullName:\n";
    auto sortedOperators = storage.get_all<Operator>(
        order_by(&Operator::fullName).asc()
    );
    for (const auto& item : sortedOperators)
    {
        std::cout << item.id << " | " << item.fullName << '\n';
    }

    // Три найдовші дзвінки
    std::cout << "\n3. Top 3 call durations:\n";
    auto longestCalls = storage.get_all<CallRecord>(
        order_by(&CallRecord::durationSeconds).desc(),
        limit(3)
    );
    for (const auto& record : longestCalls)
    {
        std::cout << record.id << " | "
            << record.topic << " | "
            << record.durationSeconds << '\n';
    }

    // Поєднання фільтрації та сортування
    std::cout << "\nTechnical Support operators sorted by fullName:\n";
    auto techSupport = storage.get_all<Operator>(
        where(c(&Operator::department) == "Technical Support"),
        order_by(&Operator::fullName).asc()
    );
    for (const auto& item : techSupport)
    {
        std::cout << item.id << " | " << item.fullName << '\n';
    }
}

void runAggregateDemo()
{
    using namespace sqlite_orm;
    auto storage = createStorage("callcenter_queries.db");
    storage.sync_schema();

    std::cout << "\nAGGREGATE QUERY DEMONSTRATION\n";

    // Кількість дзвінків
    auto callCount = storage.count<CallRecord>();
    std::cout << "\nCallRecord count: " << callCount << '\n';

    // Загальна тривалість дзвінків 
    auto totalDuration = storage.sum(&CallRecord::durationSeconds);
    std::cout << "\nTotal call duration: " << (totalDuration ? *totalDuration : 0.0) << '\n';

    // Середня тривалість дзвінка
    auto averageDuration = storage.avg(&CallRecord::durationSeconds);
    std::cout << "\nAverage call duration: " << averageDuration << '\n';

    // Групування за оператором
    std::cout << "\nCall duration by operator:\n";
    auto durationByOperator = storage.select(
        columns(
            &CallRecord::operatorId,
            sum(&CallRecord::durationSeconds)
        ),
        group_by(&CallRecord::operatorId)
    );

    for (const auto& [operatorId, durationPtr] : durationByOperator)
    {
        std::cout << "Operator ID: " << operatorId
            << " Total duration: " << (durationPtr ? *durationPtr : 0.0) << '\n';
    }
}

void runJoinDemo()
{
    using namespace sqlite_orm;
    auto storage = createStorage("callcenter_queries.db");
    storage.sync_schema();

    std::cout << "\nJOIN QUERY DEMONSTRATION\n";

    // Об'єднання Operator і CallRecord
    std::cout << "\nOperator call history:\n";
    auto records = storage.select(
        columns(
            &Operator::fullName,
            &CallRecord::callDate,
            &CallRecord::topic,
            &CallRecord::durationSeconds
        ),
        inner_join<CallRecord>(
            on(c(&Operator::id) == &CallRecord::operatorId)
        ),
        order_by(&CallRecord::id)
    );

    for (const auto& [name, date, topic, duration] : records)
    {
        std::cout << name << " | "
            << date << " | "
            << topic << " | "
            << duration << '\n';
    }

    // Об'єднання таблиць із фільтрацією
    std::cout << "\nCalls with duration over 600 seconds:\n";
    auto longRecords = storage.select(
        columns(
            &Operator::fullName,
            &CallRecord::topic,
            &CallRecord::durationSeconds
        ),
        inner_join<CallRecord>(
            on(c(&Operator::id) == &CallRecord::operatorId)
        ),
        where(c(&CallRecord::durationSeconds) > 600.0),
        order_by(&CallRecord::durationSeconds).desc()
    );

    for (const auto& [name, topic, duration] : longRecords)
    {
        std::cout << name << " | "
            << topic << " | "
            << duration << '\n';
    }
}

// Транзакції та забезпечення цілісності даних

void runTransactionDemo()
{
    auto storage = createStorage("callcenter_queries.db");
    storage.on_open = [](sqlite3* db)
        {
            if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                throw std::runtime_error("Cannot enable foreign keys");
            }
        };
    storage.sync_schema();

    std::cout << "\nTRANSACTION DEMONSTRATION\n";

    auto operatorCountBefore = storage.count<Operator>();
    auto callCountBefore = storage.count<CallRecord>();

    std::cout << "\nBefore transaction:\n";
    std::cout << "Operator: " << operatorCountBefore << '\n';
    std::cout << "CallRecord: " << callCountBefore << '\n';

    bool committed = storage.transaction([&]()
        {
            Operator op{ 0, "Yurii Kravchuk", "Technical Support", "online" };
            int operatorId = storage.insert(op);

            CallRecord call{ 0, operatorId, "2026-10-25", "Initial onboarding call", 300.0 };
            storage.insert(call);

            std::cout << "\nRecords inserted inside transaction\n";
            std::cout << "Operator ID: " << operatorId << '\n';

            return true; 
        });

    std::cout << "\nTransaction committed: " << std::boolalpha << committed << '\n';

    std::cout << "\nAfter transaction:\n";
    std::cout << "Operator: " << storage.count<Operator>() << '\n';
    std::cout << "CallRecord: " << storage.count<CallRecord>() << '\n';
}

void runRollbackDemo()
{
    auto storage = createStorage("callcenter_queries.db");
    storage.on_open = [](sqlite3* db)
        {
            if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                throw std::runtime_error("Cannot enable foreign keys");
            }
        };
    storage.sync_schema();

    std::cout << "\nROLLBACK DEMONSTRATION\n";

    auto operatorBefore = storage.count<Operator>();
    auto callBefore = storage.count<CallRecord>();

    std::cout << "\nBefore transaction:\n";
    std::cout << "Operator: " << operatorBefore << '\n';
    std::cout << "CallRecord: " << callBefore << '\n';

    try
    {
        storage.transaction([&]()
            {
                Operator op{ 0, "Cooling Operator", "Sales", "online" };
                int operatorId = storage.insert(op);
                std::cout << "\nOperator inserted, ID: " << operatorId << '\n';

                CallRecord call{ 0, -1, "2026-10-26", "Initial call", 150.0 };
                storage.insert(call);

                return true;
            });
    }
    catch (const std::exception& ex)
    {
        std::cout << "\nTransaction failed: " << ex.what() << '\n';
        std::cout << "Changes rolled back\n";
    }

    std::cout << "\nAfter transaction:\n";
    std::cout << "Operator: " << storage.count<Operator>() << '\n';
    std::cout << "CallRecord: " << storage.count<CallRecord>() << '\n';

    using namespace sqlite_orm;
    auto coolingOperators = storage.get_all<Operator>(
        where(c(&Operator::fullName) == "Cooling Operator")
    );
    std::cout << "\nCooling Operator exists: "
        << std::boolalpha << !coolingOperators.empty() << '\n';
}

void runWithoutTransactionDemo()
{
    const std::string databaseFile = "callcenter_no_transaction.db";
    std::filesystem::remove(databaseFile);

    auto storage = createStorage(databaseFile);
    storage.on_open = [](sqlite3* db)
        {
            if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                throw std::runtime_error("Cannot enable foreign keys");
            }
        };
    storage.sync_schema();

    std::cout << "\nWITHOUT TRANSACTION\n";

    try
    {
        Operator op{ 0, "Cooling Operator", "Sales", "online" };
        int operatorId = storage.insert(op);
        std::cout << "\nOperator inserted, ID: " << operatorId << '\n';

        CallRecord call{ 0, -1, "2026-10-26", "Initial call", 150.0 };
        storage.insert(call);
    }
    catch (const std::exception& ex)
    {
        std::cout << "\nOperation failed: " << ex.what() << '\n';
    }

    std::cout << "\nAfter operations:\n";
    std::cout << "Operator: " << storage.count<Operator>() << '\n';
    std::cout << "CallRecord: " << storage.count<CallRecord>() << '\n';
}

int main()
{
    try
    {
        std::filesystem::remove("callcenter.db");

        runRawSQLiteDemo();
        runOrmDemo();
        runRelationshipDemo();
        runRepositoryDemo();

        prepareTestData();
        runQueryDemo();
        runAggregateDemo();
        runJoinDemo();

        runTransactionDemo();
        runRollbackDemo();
        runWithoutTransactionDemo();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Application error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}