#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <mongoose.h>
#include <yuarel.h>

#include "Request.h"

static const int MAX_QUERY_STRING_LENGTH = 32768;
static const int MAX_QUERY_STRING_ITEMS = 200;

static int lowercase(const char *s) {
  return tolower(* (const unsigned char *) s);
}

static int mg_strncasecmp(const char *s1, const char *s2, size_t len) {
  int diff = 0;

  if (len > 0)
    do {
      diff = lowercase(s1++) - lowercase(s2++);
    } while (diff == 0 && s1[-1] != '\0' && --len > 0);

  return diff;
}

static void mg_strlcpy(register char *dst, register const char *src, size_t n) {
  for (; *src != '\0' && n > 1; n--) {
    *dst++ = *src++;
  }
  *dst = '\0';
}

/*
static int mg_strcasecmp(const char *s1, const char *s2) {
  int diff;

  do {
    diff = lowercase(s1++) - lowercase(s2++);
  } while (diff == 0 && s1[-1] != '\0');

  return diff;
}
*/

static const char *mg_strcasestr(const char *big_str, const char *small_str) {
  int i, big_len = strlen(big_str), small_len = strlen(small_str);

  for (i = 0; i <= big_len - small_len; i++) {
    if (mg_strncasecmp(big_str + i, small_str, small_len) == 0) {
      return big_str + i;
    }
  }

  return NULL;
}

static int mg_get_cookie(const char *cookie_header, const char *var_name,
                  char *dst, size_t dst_size) {
  const char *s, *p, *end;
  int name_len, len = -1;

  if (dst == NULL || dst_size == 0) {
    len = -2;
  } else if (var_name == NULL || (s = cookie_header) == NULL) {
    len = -1;
    dst[0] = '\0';
  } else {
    name_len = (int) strlen(var_name);
    end = s + strlen(s);
    dst[0] = '\0';

    for (; (s = mg_strcasestr(s, var_name)) != NULL; s += name_len) {
      if (s[name_len] == '=') {
        s += name_len + 1;
        if ((p = strchr(s, ' ')) == NULL)
          p = end;
        if (p[-1] == ';')
          p--;
        if (*s == '"' && p[-1] == '"' && p > s + 1) {
          s++;
          p--;
        }
        if ((size_t) (p - s) < dst_size) {
          len = p - s;
          mg_strlcpy(dst, s, (size_t) len + 1);
        } else {
          len = -3;
        }
        break;
      }
    }
  }
  return len;
}

namespace Mongoose
{
    Request::Request(struct mg_connection *connection, http_message *message, bool isMultipart):
        mIsValid(true),
        mIsMultipartRequest(isMultipart),
        mConnection(connection)
    {
        mMethod = std::string(message->method.p, message->method.len);
        mUrl = std::string(message->uri.p, message->uri.len);
        mQuerystring = std::string(message->query_string.p, message->query_string.len);

        for(int i = 0; i < MG_MAX_HTTP_HEADERS && message->header_names[i].len != 0; i++)
        {
            mHeaders[message->header_names[i].p] = message->header_values[i].p;
        }

        if(!mIsMultipartRequest)
        {
            mBody = std::string(message->body.p, message->body.len);
            std::string data;

            if (mMethod == "GET")
            {
                data = mQuerystring;
            }
            else if (mMethod == "POST")
            {
                data = mBody;
            }
            else
            {
                //Nothing to do.
            }

            char querystring[MAX_QUERY_STRING_LENGTH] = {0};
            strncpy(querystring, data.c_str(), MAX_QUERY_STRING_LENGTH);

            //TODO: Replace yuarel with https://github.com/bartgrantham/qs_parse
            struct yuarel_param params[MAX_QUERY_STRING_ITEMS];
            int count = yuarel_parse_query(querystring, '&', params, MAX_QUERY_STRING_ITEMS);
            for(int i = 0; i < count; i++)
            {
                struct yuarel_param p = params[i];
                int keyLen = strnlen(p.key, data.size());
                int valLen = strnlen(p.val, data.size() - keyLen);
                std::string key = std::string(p.key, keyLen);
                std::string value = std::string(p.val, valLen);
                mVariables[key] = value;
            }
        }

        //FIXME: Parse cookies, and improve Cookie handling code with another actual Cookie class
    }

    Request::~Request()
    {
        for (const auto& entity : mMultipartEntities)
        {
            //Remove all the temporary files created for this request
            if (entity.filePath.size() > 0)
            {
                remove(entity.filePath.c_str());
            }
        }
    }

    bool Request::hasVariable(const std::string &key) const
    {
        return mVariables.find(key) != mVariables.end();
    }

    std::string Request::getVariable(const std::string &key, const std::string &fallback) const
    {
        std::string result = fallback;

        if (hasVariable(key))
        {
            result = mVariables.at(key);
        }

        return result;
    }


#ifdef ENABLE_REGEX_URL
    smatch Request::getMatches()
    {
        return matches;
    }

    bool Request::match(string pattern)
    {
        key = method + ":" + url;
        return regex_match(key, matches, regex(pattern));
    }
#endif


    bool Request::hasCookie(const std::string &key) const
    {
        int i;
        char dummy[10];

        for (auto header: mHeaders)
        {
            if (header.first.find("Cookie") != std::string::npos)
            {
                if (mg_get_cookie(header.second.c_str(), key.c_str(), dummy, sizeof(dummy)) != -1)
                {
                    return true;
                }
            }
        }

        return false;
    }

    std::string Request::getCookie(const std::string &key, const std::string &fallback) const
    {
        std::string output;
        int i;
        int size = 1024;
        int ret;
        char *buffer = new char[size];
        char dummy[10];
        const char *place = NULL;

        for (auto header: mHeaders)
        {
            if (header.first.find("Cookie") != std::string::npos)
            {
                if (mg_get_cookie(header.second.c_str(), key.c_str(), dummy, sizeof(dummy)) != -1)
                {
                    place = header.second.c_str();
                    break;
                }
            }
        }

        if (place == NULL) {
            return fallback;
        }

        do {
            ret = mg_get_cookie(place, key.c_str(), buffer, size);

            if (ret == -2) {
                size *= 2;
                delete[] buffer;
                buffer = new char[size];
            }
        } while (ret == -2);

        output = std::string(buffer);
        delete[] buffer;

        return output;
    }

    bool Request::hasHeader(const std::string &key) const
    {
        return mHeaders.find(key) != mHeaders.end();
    }

    std::string Request::getHeaderValue(const std::string& key) const
    {
      std::string result;

      if (hasHeader(key))
      {
          result = mHeaders.at(key);
      }

      return result;
    }

    std::map<std::string, std::string> Request::headers() const
    {
        return mHeaders;
    }

    std::string Request::url() const
    {
        return mUrl;
    }

    std::string Request::method() const
    {
        return mMethod;
    }

    std::string Request::body() const
    {
        return mBody;
    }

    bool Request::isValid() const
    {
        return mIsValid;
    }

    void Request::setIsValid(bool value)
    {
        mIsValid = value;
    }

    bool Request::isMultipartRequest() const
    {
        return mIsMultipartRequest;
    }

    std::vector<Request::MultipartEntity> Request::multipartEntities() const
    {
        return mMultipartEntities;
    }

    void Request::setMultipartEntities(const std::vector<Request::MultipartEntity> &entities)
    {
        mMultipartEntities = entities;

        //For easier access
        for (const auto& entity : entities)
        {
            mVariables[entity.variableName] = entity.filePath.size() > 0
                                              ? entity.filePath
                                              : entity.variableData;
        }
    }

}
