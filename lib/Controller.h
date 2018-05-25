#ifndef _MONGOOSE_CONTROLLER_H
#define _MONGOOSE_CONTROLLER_H

#include <functional>
#include <map>
#include <memory>
#include <vector>

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
    class AbstractRequestPreprocessor;
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
             * @param Request - the incoming request to preProcess
             * @param Response - the response
             */
            virtual bool preProcess(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response);

            /**
             * @brief process - Called to process a request
             * @param Request the request
             * @return Response the created response, or NULL if the controller
             *         does not handle this request
             */
            virtual bool process(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response);

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
             * @brief registerPreprocessor - a pre processor can alter the request/response params before processing.
             * Useful for keeping track of session variables etc...
             * @param preprocessor
             */
            void registerPreprocessor(AbstractRequestPreprocessor* preprocessor);

            /**
             * @brief deregisterPreprocessor - unregister preprocessor from executing
             * @param preprocessor
             */
            void deregisterPreprocessor(AbstractRequestPreprocessor* preprocessor);

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
            std::vector<AbstractRequestPreprocessor*> mPreprocessors;
    };
}

#endif
