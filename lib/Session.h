#ifndef _MONGOOSE_SESSION_H
#define _MONGOOSE_SESSION_H

#include <map>
#include <string>

/**
 * A session contains the user specific values
 */
namespace Mongoose
{
class Session
{
public:
    Session();

    /**
    * @brief setValue Set the value of a session variable
    * @param string the name of the variable
    * @param string the value of the variable
    */
    void setValue(const std::string& key, const std::string& value);

    /**
    * @brief unsetValue Unset a session varaible
    * @param string the variable name
    */
    void unsetValue(const std::string& key);

    /**
    * @brief hasValue Check if the given variable exists
    * @param string the name of the variable
    */
    bool hasValue(const std::string& key) const;

    /**
    * @brief get Try to get the value for the given variable
    * @param string the name of the variable
    * @param string the fallback value
    * @return string the value of the variable if it exists, fallback else
    */
    std::string value(const std::string& key, const std::string& fallback = "") const;

    /**
    * @brief Pings the session, this will update the creation date to now
    * and "keeping it alive"
    */
    void ping();

    /**
    * @brief Returns the session age, in seconds
    * @return int the number of sessions since the last activity of the session
    */
    int getAge();

protected:
    std::map<std::string, std::string> mValues;
    int mDate;
};
}

#endif
