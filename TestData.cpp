#include "TestData.h"
#include "Database.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <sqlite3.h>

void prepareTestData()
{
    const std::string databaseFile = "callcenter_queries.db";

    std::filesystem::remove(databaseFile);

    auto storage = createStorage(databaseFile);
    storage.on_open = [](sqlite3* db)
        {
            if (sqlite3_exec(
                db,
                "PRAGMA foreign_keys = ON;",
                nullptr,
                nullptr,
                nullptr
            ) != SQLITE_OK)
            {
                throw std::runtime_error("Cannot enable foreign keys");
            }
        };

    storage.open_forever();
    storage.sync_schema();

    std::vector<Operator> operatorList = {
        {0, "Ivan Petrenko",     "Technical Support", "online"},
        {0, "Olena Kovalenko",   "Sales",             "online"},
        {0, "Maksym Bondar",     "Billing",           "busy"},
        {0, "Anna Shevchenko",   "Technical Support", "online"},
        {0, "Dmytro Tkachenko",  "Sales",             "offline"}
    };

    std::vector<int> operatorIds;
    for (const auto& op : operatorList)
    {
        operatorIds.push_back(storage.insert(op));
    }

    std::vector<CallRecord> callList = {
        {0, operatorIds[0], "2026-09-01", "Router configuration",        420.0},
        {0, operatorIds[0], "2026-09-15", "Internet connection drop",    650.0},
        {0, operatorIds[0], "2026-10-10", "Static IP setup",             910.0},

        {0, operatorIds[1], "2026-08-10", "Tariff upgrade inquiry",      240.0},
        {0, operatorIds[1], "2026-09-12", "New corporate contract",      840.0},
        {0, operatorIds[1], "2026-10-05", "Discount promo consultation", 190.0},

        {0, operatorIds[2], "2026-07-20", "Invoice discrepancy",         530.0},
        {0, operatorIds[2], "2026-08-25", "Refund request",              780.0},
        {0, operatorIds[2], "2026-10-12", "Auto-payment failure",        310.0},

        {0, operatorIds[3], "2026-06-18", "Optical modem replacement",   560.0},
        {0, operatorIds[3], "2026-08-30", "Speed test troubleshooting",  490.0},
        {0, operatorIds[3], "2026-10-18", "Firewall port forwarding",    1120.0},

        {0, operatorIds[4], "2026-05-14", "Equipment rental terms",      360.0},
        {0, operatorIds[4], "2026-07-22", "VIP package presentation",    1250.0},
        {0, operatorIds[4], "2026-09-28", "Contract renewal",            290.0}
    };

    for (const auto& call : callList)
    {
        storage.insert(call);
    }

    std::cout << "\nTest database prepared\n";
    std::cout << "Operator records: " << storage.count<Operator>() << '\n';
    std::cout << "CallRecord records: " << storage.count<CallRecord>() << '\n';
}