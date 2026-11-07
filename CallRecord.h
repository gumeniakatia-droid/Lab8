#pragma once
#include <string>

struct CallRecord
{
    int id = 0;
    int operatorId = 0;
    std::string callDate;
    std::string topic;
    double durationSeconds = 0.0;
};