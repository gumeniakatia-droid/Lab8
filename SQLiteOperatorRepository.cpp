#include "SQLiteOperatorRepository.h"
#include <stdexcept>
#include <sqlite3.h>

SQLiteOperatorRepository::SQLiteOperatorRepository(const std::string& databaseFile)
    : storage_(createStorage(databaseFile))
{
    storage_.on_open = [](sqlite3* db)
        {
            int rc = sqlite3_exec(
                db,
                "PRAGMA foreign_keys = ON;",
                nullptr,
                nullptr,
                nullptr
            );
            if (rc != SQLITE_OK)
            {
                throw std::runtime_error("Cannot enable foreign key constraints");
            }
        };

    storage_.sync_schema();
}

int SQLiteOperatorRepository::add(const Operator& op)
{
    return storage_.insert(op);
}

std::optional<Operator> SQLiteOperatorRepository::findById(int id)
{
    return storage_.get_optional<Operator>(id);
}

std::vector<Operator> SQLiteOperatorRepository::findAll()
{
    return storage_.get_all<Operator>();
}

bool SQLiteOperatorRepository::update(const Operator& op)
{
    auto existing = storage_.get_optional<Operator>(op.id);
    if (!existing)
    {
        return false;
    }

    storage_.update(op);
    return true;
}

bool SQLiteOperatorRepository::remove(int id)
{
    auto existing = storage_.get_optional<Operator>(id);
    if (!existing)
    {
        return false;
    }

    storage_.remove<Operator>(id);
    return true;
}