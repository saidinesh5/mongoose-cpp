#ifndef _MONGOOSE_SESSIONS_H
#define _MONGOOSE_SESSIONS_H

#include <memory>

#include "Request.h"
#include "AbstractRequestCoprocessor.h"
#include "Response.h"
#include "Session.h"


/**
 * A session contains the user specific values
 */ 
namespace Mongoose
{
    class Sessions: public AbstractRequestCoprocessor
    {
        public:
            Sessions(const std::string& key = "sessid",
                     Controller *controller = nullptr,
                     Server *server = nullptr);
            virtual ~Sessions();

            /**
             * Gets the session ID of a certain request,
             * this will look in the cookies and create it if
             * necessary
             *
             * @param Request the request
             * @param Response the response in which the cookie should be write
             *
             * @return string the session ID for this request
             */
            std::string getId(const std::shared_ptr<Request> &request, const std::shared_ptr<Response> &response);

            /**
             * Gets the session for a certain request
             *
             * @param Request the request
             * @param Response the response inwhich the cookie should be write
             *
             * @return Session the session corresponding
             */
            Session* get(const std::shared_ptr<Request> &request, const std::shared_ptr<Response> &response);

            /**
             * Remove all the sessions older than age
             *
             * @param int the age of the too old sessions in second
             */
            void garbageCollect(int oldAge = 3600);

            bool preProcess(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response) override;

            unsigned int gcDivisor() const { return mGcDivisor; }
            void setGcDivisor(unsigned int divisor) { mGcDivisor = divisor; }

        private:

            unsigned int mGcDivisor;
            int mGcCounter;
            std::map<std::string, Session*> mSessions;
            std::string mKey;
    };
}

#endif
