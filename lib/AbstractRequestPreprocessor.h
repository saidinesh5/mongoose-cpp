#ifndef _MONGOOSE_REQUEST_HANDLER_H
#define _MONGOOSE_REQUEST_HANDLER_H

#include "Request.h"
#include "Response.h"
#include <string>
#include <memory>

namespace Mongoose
{
    class Server;
    class Controller;
    class AbstractRequestPreprocessor
    {
        public:
            AbstractRequestPreprocessor(Controller *controller = nullptr, Server *server = nullptr):
                mController(controller),
                mServer(server)
            {
            }

            Controller* controller() const { return mController; }
            void setController(Controller *controller) { mController = controller; }

            Server* server() const { return mServer; }
            void setServer(Server *server) { mServer = server; }

            virtual bool preProcess(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response) = 0;

        protected:
            Server *mServer;
            Controller *mController;
    };
}

#endif
