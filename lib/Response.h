#ifndef _MONGOOSE_RESPONSE_H
#define _MONGOOSE_RESPONSE_H

#include <atomic>
#include <map>
#include <string>

struct mg_connection;;

#ifdef HAS_JSON11
#include <json11.hpp>
#endif

#define HTTP_OK 200
#define HTTP_NOT_FOUND 404
#define HTTP_FORBIDDEN 403
#define HTTP_SERVER_ERROR 500

/**
 * A response to a request
 */
namespace Mongoose
{
class Response
{
public:
    explicit Response(struct mg_connection *connection);
    ~Response();

    bool hasHeader(const std::string& key) const;
    std::string getHeaderValue(const std::string& key) const;
    void setHeader(const std::string& key, const std::string& value);
    std::map<std::string, std::string> headers() const;

    void setCookie(const std::string& key, const std::string& value);

    int code() const;
    void setCode(int code);

    std::string body() const;
    void setBody(const std::string& body);

   /**
    * @brief sends the body as plain text and attempts to close the connection
    * @return
    */
    bool send();
    bool send(int statusCode, const std::string& body);
    bool send(const std::string& body);
    bool sendHtml(const std::string& body);
    bool sendFile(const std::string& path, const std::string& type = "text/plain");
    bool sendError(const std::string& message);
    bool sendRedirect(const std::string& url, bool permanent = false);

#ifdef HAS_JSON11
    bool sendJson(const json11::Json &body);
#endif

    bool isValid() const;
    void setIsValid(bool value);

private:

    std::string headerString() const;

    int mCode;
    std::map<std::string, std::string> mHeaders;
    std::string mBody;
    struct mg_connection *mConnection;
    std::atomic_bool mIsValid;
};
}

#endif
