#pragma once
#include <optional>
#include <vector>
#include "Operator.h"

class IOperatorRepository
{
public:
    virtual ~IOperatorRepository() = default;

    // Create
    virtual int add(const Operator& op) = 0;

    // Read
    virtual std::optional<Operator> findById(int id) = 0;
    virtual std::vector<Operator> findAll() = 0;

    // Update
    virtual bool update(const Operator& op) = 0;

    // Delete
    virtual bool remove(int id) = 0;
};