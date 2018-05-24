#include <time.h>
#include <iostream>
#include <string>
#include "Session.h"

namespace Mongoose
{
Session::Session()
{
    ping();
}

void Session::ping()
{
    mDate = time(NULL);
}

void Session::setValue(const std::string &key, const std::string &value)
{
    mValues[key] = value;
}

void Session::unsetValue(const std::string &key)
{
    mValues.erase(key);
}

bool Session::hasValue(const std::string &key) const
{
    return mValues.find(key) != mValues.end();
}

std::string Session::value(const std::string &key, const std::string &fallback) const
{
    if (hasValue(key)) {
        std::string value = mValues.at(key);
        return value;
    } else {
        return fallback;
    }
}

int Session::getAge()
{
    return time(NULL)-mDate;
}
}
