#include <cassert>

#include "Sessions.h"
#include "Utils.h"


namespace Mongoose
{
    Sessions::Sessions(const std::string &key, Controller *controller, Server *server)
        :
          AbstractRequestCoprocessor(controller, server),
          mGcDivisor(100),
          mGcCounter(0),
          mSessions(),
          mKey(key)
    {
    }

    Sessions::~Sessions()
    {
        std::map<std::string, Session *>::iterator it;
        for (it=mSessions.begin(); it!=mSessions.end(); it++)
        {
            delete (*it).second;
        }
    }

    std::string Sessions::getId(const std::shared_ptr<Request> &request, const std::shared_ptr<Response> &response)
    {
        if (request->hasCookie(mKey)) {
            return request->getCookie(mKey);
        } else {

            std::string newCookie = Utils::randomAlphanumericString(30);
            response->setCookie(mKey, newCookie);
            return newCookie;
        }
    }

    Session* Sessions::get(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response)
    { 
        std::string id = getId(request, response);
        Session *session = NULL;
        
        if (mSessions.find(id) != mSessions.end()) {
            session = mSessions[id];
        } else {
            session = new Session();
            mSessions[id] = session;
        }

        return session;
    }

    void Sessions::garbageCollect(int oldAge)
    {
        std::vector<std::string> deleteList;
        std::map<std::string, Session*>::iterator it;
        std::vector<std::string>::iterator vit;

        for (it=mSessions.begin(); it!=mSessions.end(); it++) {
            std::string name = (*it).first;
            Session *session = (*it).second;

            if (session->getAge() > oldAge) {
                delete session;
                deleteList.push_back(name);
            }
        }

        for (vit=deleteList.begin(); vit!=deleteList.end(); vit++) {
            mSessions.erase(*vit);
        }
    }

    bool Sessions::preProcess(const std::shared_ptr<Request> &request, const std::shared_ptr<Response> &response)
    {
        mGcCounter++;

        if (mGcCounter > mGcDivisor)
        {
            mGcCounter = 0;
            garbageCollect();
        }

        get(request, response)->ping();
        return true;
    }
}
