#pragma once
#include <string>
#include "IOperatorRepository.h"
#include "Database.h"

class SQLiteOperatorRepository : public IOperatorRepository
{
public:
    explicit SQLiteOperatorRepository(const std::string& databaseFile);

    int add(const Operator& op) override;
    std::optional<Operator> findById(int id) override;
    std::vector<Operator> findAll() override;
    bool update(const Operator& op) override;
    bool remove(int id) override;

private:
    using Storage = decltype(createStorage(""));
    Storage storage_;
};