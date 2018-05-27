#include <algorithm>
#include <iostream>

#include "Controller.h"
#include "Request.h"
#include "AbstractRequestCoprocessor.h"
#include "Response.h"

namespace Mongoose
{
    Controller::Controller(Server *server):
        mServer(server),
        mPrefix("")
    {
    }

    void Controller::setup()
    {
    }
    
    Controller::~Controller()
    {
        mRoutes.clear();
    }

    bool Controller::preProcess(const std::shared_ptr<Request> &request, const std::shared_ptr<Response> &response)
    {
        bool result = true;

        for (int i = 0; i < mCoprocessors.size() && result; i++)
        {
            result = mCoprocessors[i]->preProcess(request, response);
        }

        return result;
    }

    bool Controller::process(const std::shared_ptr<Request> &request, const std::shared_ptr<Response> &response)
    {
        bool result = false;

#ifdef ENABLE_REGEX_URL
        std::map<std::string, RequestHandler *>::iterator it;
        for (it=routes.begin(); it!=routes.end(); it++)
        {
            if (request->match(it->first))
            {
              result = it->second->process(request, response);
              break;
            }   
        }
#else
        std::string key = request->method() + ":" + request->url();
        if (mRoutes.find(key) != mRoutes.end())
        {
            result = mRoutes[key](request, response);
        }
#endif
        
        return result;
    }

    bool Controller::postProcess(const std::shared_ptr<Request> &request, const std::shared_ptr<Response> &response)
    {
        bool result = true;

        for (int i = 0; i < mCoprocessors.size() && result; i++)
        {
            result = mCoprocessors[i]->postProcess(request, response);
        }

        return result;
    }


    bool Controller::handles(const std::string &method, const std::string &url) const
    {
        std::string key = method + ":" + url;
        return (mRoutes.find(key) != mRoutes.end());
    }

    bool Controller::handleRequest(const std::shared_ptr<Request> &request, const std::shared_ptr<Response> &response)
    {
        return preProcess(request, response)
               && process(request, response);
    }

    Server *Controller::server() const
    {
        return mServer;
    }

    void Controller::setServer(Server *server)
    {
        mServer = server;
    }

    std::string Controller::prefix() const
    {
        return mPrefix;
    }

    void Controller::setPrefix(const std::string &prefix)
    {
        mPrefix = prefix;
    }

    void Controller::registerCoprocessor(AbstractRequestCoprocessor *preprocessor)
    {
        if (std::find(mCoprocessors.begin(), mCoprocessors.end(), preprocessor) == mCoprocessors.end())
        {
            mCoprocessors.push_back(preprocessor);
        }
    }

    void Controller::deregisterCoprocessor(AbstractRequestCoprocessor *preprocessor)
    {
        auto position = std::find(mCoprocessors.begin(), mCoprocessors.end(), preprocessor);

        if (position != mCoprocessors.end())
        {
            mCoprocessors.erase(position);
        }
    }
            
    void Controller::registerRoute(std::string httpMethod, std::string httpRoute, RequestHandler handler)
    {
        std::string key = httpMethod + ":" + mPrefix + httpRoute;
        mRoutes[key] = handler;
        mUrls.push_back(mPrefix + httpRoute);
    }

    void Controller::deregisterRoute(std::string httpMethod, std::string httpRoute)
    {
        std::string key = httpMethod + ":" + mPrefix + httpRoute;
        if (mRoutes.find(key) != mRoutes.end())
        {
            mRoutes.erase(key);
        }

        std::string url = mPrefix + httpRoute;
        auto pos = std::find(mUrls.begin(), mUrls.end(), url);

        if (pos != mUrls.end())
        {
            mUrls.erase(pos);
        }
    }

    void Controller::dumpRoutes() const
    {
        std::cout << "Routes:" << std::endl;
        for (const auto &route : mRoutes)
        {
            std::cout << "    " << route.first << std::endl;
        }

    }

    std::vector<std::string> Controller::urls() const
    {
        return mUrls;
    }
}
