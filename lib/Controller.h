#ifndef _MONGOOSE_CONTROLLER_H
#define _MONGOOSE_CONTROLLER_H

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <string>

//Helper define for binding class methods
#define addRoute(httpMethod, httpRoute, className, methodName) \
    registerRoute(httpMethod, httpRoute, std::bind(&className::methodName, this, std::placeholders::_1, std::placeholders::_2))

/**
 * A controller is a module that respond to requests
 *
 * You can override the preProcess, process and postProcess to answer to
 * the requests
 */
namespace Mongoose
{
    class AbstractRequestCoprocessor;
    class Server;
    class Request;
    class Response;

    typedef std::function<bool(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)> RequestHandler;

    class Controller
    {
        public:
            Controller(Server *server = nullptr);
            virtual ~Controller();

            /**
             * @brief setup - initializes routes, preprocessors etc..
             */
            virtual void setup();

            /**
             * @brief preProcess - Called before a request is processed
             * @param request - the incoming request to preProcess
             * @param response - the response
             */
            virtual bool preProcess(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response);

            /**
             * @brief process - Called to process a request
             * @param request the request
             * @return response the response
             */
            virtual bool process(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response);

            /**
             * @brief postProcess - Called after a request is processed
             * @param request - the incoming request to postProcess
             * @param response - the response - NOTE - that the response may not always be "valid" and do not attempt
             * to use any response->send*() methods while post processing
             * @return
             */
            virtual bool postProcess(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response);

            /**
              * @brief handles - check if this controller can handle a http method + url
              * @param method
              * @param url
              * @return true if this controller handles the http method + url combo
              */
             virtual bool handles(const std::string& method, const std::string& url) const;

            /**
             * @brief handleRequest - Handle a request, this will try to match the request, if this
             * controller handles it, it will preProcess, process then postProcess it
             * @param Request the incoming request
             * @return Response the created response, or NULL if the controller
             *         does not handle this request
             */
            virtual bool handleRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response);

            /**
             * @brief server
             * @return returns the reference to the server hosting this controller
             */
            Server* server() const;

            /**
             * @brief Sets the reference to the server hosting this controller
             * @param Server the hosting server
             */
            void setServer(Server *server);

            /**
             * @brief prefix - gets the controller prefix
             * @return the prefix of all urls for this controller
             */
            std::string prefix() const;

            /**
             * @brief Sets the controller prefix, for instance "/api"
             * @param string the prefix of all urls for this controller
             */
            void setPrefix(const std::string& prefix);

            /**
             * @brief registerCoprocessor - a co processor can alter the request/response params before/after processing.
             * Useful for keeping track of session variables, altering error pages, logging requests/responses etc.
             * @param preprocessor
             */
            void registerCoprocessor(AbstractRequestCoprocessor* preprocessor);

            /**
             * @brief deregisterCoprocessor - unregister preprocessor from executing
             * @param preprocessor
             */
            void deregisterCoprocessor(AbstractRequestCoprocessor* preprocessor);

            /**
             * @brief registerRoute
             * @param httpMethod - GET, POST etc..
             * @param httpRoute - http endpoint pat . like /users
             */
            void registerRoute(std::string httpMethod, std::string httpRoute, RequestHandler handler);

            /**
             * @brief deregisterRoute
             * @param httpMethod - GET, POST etc..
             * @param httpRoute - http endpoint pat . like /users
             */
            void deregisterRoute(std::string httpMethod, std::string httpRoute);


            /**
             * @brief dumpRoutes - prints all http routes registered
             */
            void dumpRoutes() const;

            /**
             * @brief urls
             * @return return all the urls handled by this controller
             */
            std::vector<std::string> urls() const;

        protected:
            Server *mServer;
            std::string mPrefix;
            std::map<std::string, RequestHandler> mRoutes;
            std::vector<std::string> mUrls;
            std::vector<AbstractRequestCoprocessor*> mCoprocessors;
    };
}

#endif
