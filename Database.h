#pragma once
#include <string>
#include <sqlite_orm/sqlite_orm.h>
#include "Operator.h"
#include "CallRecord.h"

inline auto createStorage(const std::string& filename)
{
    using namespace sqlite_orm;

    return make_storage(
        filename,
        make_table(
            "Operator",
            make_column(
                "id",
                &Operator::id,
                primary_key().autoincrement()
            ),
            make_column(
                "fullName",
                &Operator::fullName
            ),
            make_column(
                "department",
                &Operator::department
            ),
            make_column(
                "status",
                &Operator::status
            )
        ),
        make_table(
            "CallRecord",
            make_column(
                "id",
                &CallRecord::id,
                primary_key().autoincrement()
            ),
            make_column(
                "operatorId",
                &CallRecord::operatorId
            ),
            make_column(
                "callDate",
                &CallRecord::callDate
            ),
            make_column(
                "topic",
                &CallRecord::topic
            ),
            make_column(
                "durationSeconds",
                &CallRecord::durationSeconds
            ),
            foreign_key(
                &CallRecord::operatorId
            ).references(
                &Operator::id
            )
        )
    );
}