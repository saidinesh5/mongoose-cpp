#ifndef _MONGOOSE_REQUEST_H
#define _MONGOOSE_REQUEST_H

#include <atomic>
#include <map>
#include <string>
#include <vector>
#ifdef ENABLE_REGEX_URL
#include <regex>
#endif

struct mg_connection;
struct http_message;

/**
 * Request is a wrapper for the clients requests
 */
namespace Mongoose
{
class Request
{

public:

    struct MultipartEntity
    {
        std::string variableName;
        std::string variableData;
        std::string fileName;
        std::string filePath;
    };

    Request(struct mg_connection *connection, struct http_message* message, bool isMultipart = false);
    ~Request();

    bool hasVariable(const std::string& key) const;
    std::string getVariable(const std::string& key, const std::string& fallback = "") const;
    std::map<std::string, std::string> variables() const { return mVariables; }

    bool hasCookie(const std::string& key) const;
    std::string getCookie(const std::string& key, const std::string& fallback = "") const;
    std::map<std::string, std::string> cookies() const;

    bool hasHeader(const std::string& key) const;
    std::string getHeaderValue(const std::string& key) const;
    std::map<std::string, std::string> headers() const;

    std::string url() const;
    std::string method() const;
    std::string body() const;

#ifdef ENABLE_REGEX_URL
    smatch getMatches();
    bool match(string pattern);
#endif
    bool isValid() const;
    void setIsValid(bool value);

    bool isMultipartRequest() const;
    std::vector<MultipartEntity> multipartEntities() const;
    void setMultipartEntities(const std::vector<MultipartEntity>& entities);

private:

    std::atomic_bool mIsValid;
    bool mIsMultipartRequest;
    std::string mMethod;
    std::string mUrl;
    std::string mQuerystring;
    std::string mBody;

    std::map<std::string, std::string> mHeaders;
    std::map<std::string, std::string> mCookies;

    //For multipart form uploads
    std::vector<MultipartEntity> mMultipartEntities;
    std::map<std::string, std::string> mVariables;
    struct mg_connection *mConnection;
};
}

#endif
