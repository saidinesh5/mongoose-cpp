#include <fstream>
#include <sstream>
#include <mongoose.h>

#include "Response.h"


namespace Mongoose
{
    Response::Response(mg_connection *connection):
        mCode(HTTP_OK),
        mConnection(connection),
        mIsValid(true)
    {
    }
            
    Response::~Response()
    {
    }

    bool Response::hasHeader(const std::string &key) const
    {
        return mHeaders.find(key) != mHeaders.end();
    }

    std::string Response::getHeaderValue(const std::string &key) const
    {
        std::string result;

        if (hasHeader(key))
        {
            result = mHeaders.at(key);
        }

        return result;
    }
            
    void Response::setHeader(const std::string &key, const std::string &value)
    {
        mHeaders[key] = value;
    }

    std::map<std::string, std::string> Response::headers() const
    {
        return mHeaders;
    }

    void Response::setCookie(const std::string &key, const std::string &value)
    {
        std::ostringstream definition;
        definition << key << "=" << value << "; path=/";
        setHeader("Set-cookie", definition.str());
    }

    int Response::code() const
    {
        return mCode;
    }

    void Response::setCode(int code)
    {
        mCode = code;
    }

    std::string Response::body() const
    {
        return mBody;
    }

    void Response::setBody(const std::string &body)
    {
        mBody = body;
    }

    bool Response::send()
    {
        if (!mIsValid)
            return false;

        if (mHeaders.find("Content-Type") == mHeaders.end())
        {
            mHeaders["Content-Type"] = "text/plain";
        }

        if (mHeaders.find("Content-Length") == mHeaders.end())
        {
            std::ostringstream length;
            length << mBody.size();
            setHeader("Content-Length", length.str());
        }

        std::string headers = headerString();

        if(mIsValid)
        {
            mg_printf(mConnection, "%s", headers.c_str());
            mg_printf(mConnection, "%s", mBody.c_str());
        }
        mConnection->flags |= MG_F_SEND_AND_CLOSE;
        mIsValid = false;
        return true;
    }

    bool Response::send(int statusCode, const std::string &body)
    {
        if (mIsValid)
        {
            setCode(statusCode);
            mBody = body;
            return send();
        }
        return false;
    }

    bool Response::send(const std::string &body)
    {
        if (mIsValid)
        {
            mBody = body;
            return send();
        }
        return false;
    }

    bool Response::sendHtml(const std::string &body)
    {
        if (mIsValid)
        {
            mHeaders["Content-Type"] = "text/html";
            mBody = body;
            return send();
        }
        return false;
    }

    bool Response::sendFile(const std::string& path, const std::string& type)
    {
        std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);

        if (!in.good() || !mIsValid)
            return false;

        //TODO: make type optional
        char buf[2048];
        size_t n;
        size_t fileSize = in.tellg();

        mHeaders["Content-Type"] = type;
        mHeaders["Content-Length"] = fileSize;

        std::string headers = headerString();
        mg_printf(mConnection, "%s", headers.c_str());

        FILE *fp = fopen(path.c_str(), "rb");

        while (mIsValid && ((n = mg_fread(buf, 1, sizeof(buf), fp)) > 0))
        {
          mg_send(mConnection, buf, n);
        }

        fclose(fp);
        mConnection->flags |= MG_F_SEND_AND_CLOSE;
        mIsValid = false;
        return true;
    }

    bool Response::sendError(const std::string &message)
    {
        if (mIsValid)
        {
            setCode(HTTP_SERVER_ERROR);
            mBody = "[500] Server internal error: " + message;
            return send();
        }

        return false;
    }

    bool Response::sendRedirect(const std::string &url, bool permanent)
    {
        bool result = false;

        if (mIsValid)
        {
            mg_http_send_redirect(mConnection, (permanent? 301 : 302),  mg_mk_str(url.c_str()), mg_mk_str(NULL));
            mConnection->flags |= MG_F_SEND_AND_CLOSE;
            mIsValid = false;
            result = true;
        }

        return true;
    }

#ifdef HAS_JSON11
    bool Response::sendJson(const json11::Json& body)
    {
        if (mIsValid)
        {
            setHeader("Content-Type", "application/json");
            mBody = body.dump();
            return send();
        }

        return false;
    }
#endif

    bool Response::isValid() const
    {
        return mIsValid;
    }

    void Response::setIsValid(bool value)
    {
        mIsValid = value;
    }

    std::string Response::headerString() const
    {
        std::ostringstream data;
        data << "HTTP/1.0 " << mCode << "\r\n";

        for (const auto& header : mHeaders)
        {
            data << header.first << ": " << header.second << "\r\n";
        }

        data << "\r\n";

        return data.str();
    }

}
