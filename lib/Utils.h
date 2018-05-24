#ifndef _MONGOOSE_UTILS_H
#define _MONGOOSE_UTILS_H

#include <iostream>

namespace Mongoose
{
    class Utils
    {
        public:
            static std::string htmlEntities(const std::string& data);
            static void sleep(int ms);
            static int getTime();
            static std::string randomAlphanumericString(int length = 30);
            static std::string sanitizeFilename(const std::string& filename);
    };
}

#endif

