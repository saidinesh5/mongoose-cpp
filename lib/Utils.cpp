#include <iostream>
#include <sstream>
#include <ctype.h>

#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#endif

#include "Utils.h"

static char charset[] = "abcdeghijklmnpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
#define CHARSET_SIZE (sizeof(charset)/sizeof(char))

namespace Mongoose
{
    std::string Utils::htmlEntities(const std::string& data)
    {
        std::string buffer;
        buffer.reserve(data.size());

        for(size_t pos = 0; pos != data.size(); ++pos) {
            switch(data[pos]) {
                case '&':  buffer.append("&amp;");       break;
                case '\"': buffer.append("&quot;");      break;
                case '\'': buffer.append("&apos;");      break;
                case '<':  buffer.append("&lt;");        break;
                case '>':  buffer.append("&gt;");        break;
                default:   buffer.append(1, data[pos]); break;
            }
        }

        return buffer;
    }

    void Utils::sleep(int ms)
    {
#ifdef WIN32
	Sleep(ms);
#else
    usleep(1000 * ms);
#endif
    }

    int Utils::getTime()
    {
#ifdef WIN32
    time_t ltime;
    time(&ltime);
    return ltime;
#else
    return time(NULL);
#endif
    }

    std::string Utils::randomAlphanumericString(int length)
    {
        std::string result;
        result.reserve(length);

        for (int i=0; i < length; i++) {
            result[i] = charset[rand()%CHARSET_SIZE];
        }

        return result;
    }

    std::string Utils::sanitizeFilename(const std::string &filename)
    {
        //Only whitelist [0-9A-Za-z.] and replace non compliant characters with a "_"
        std::string result;
        result.reserve(filename.size());

        for(int i = 0; i < filename.size() && result.size() < 256; i++)
        {
            auto c = filename[i];
            if(isalnum(c) || c == '.')
            {
                result.push_back(c);
            }
            else
            {
                if (result.back() != '_')
                {
                    result.push_back('_');
                }
            }
        }

        if (result == "..")
        {
            result = Utils::randomAlphanumericString(30);
        }

        return result;
    }

}
