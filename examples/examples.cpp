#ifndef _MSC_VER
#include <unistd.h>
#include <stdlib.h>
#else
#include <time.h>
#endif

#include <thread>
#include <chrono>
#include <cassert>
#include <sstream>
#include <signal.h>

#include "Server.h"
#include "Sessions.h"
#include "Controller.h"
#include "Utils.h"

using namespace Mongoose;

class MyController : public Controller
{
    Sessions mSessions;

    public: 
        bool hello(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            std::stringstream body;
            body << "Hello " << req->getVariable("name", "... what's your name ?\n");
            res->send(body.str());
            return true;
        }

        bool hello_delayed(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            std::thread([=]
            {
                int duration = std::stoi(req->getVariable("duration", "3"));
                std::this_thread::sleep_for(std::chrono::seconds(duration));
                res->send("Hello after " + std::to_string(duration) + " seconds\n");
            }).detach();

            return true;
        }

        bool form(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            std::stringstream responseBody;
            responseBody << "<form method=\"post\">" << std::endl;
            responseBody << "<input type=\"text\" name=\"test\" /><br >" << std::endl;
            responseBody << "<input type=\"submit\" value=\"Envoyer\" />" << std::endl;
            responseBody << "</form>" << std::endl;

            return res->send(responseBody.str());
        }

        bool formPost(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            std::stringstream responseBody;
            responseBody << "Test=" << req->getVariable("test", "(unknown)");
            return res->send(responseBody.str());
        }

        bool session(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            Session *session = mSessions.get(req, res);
            std::stringstream responseBody;

            if (session->hasValue("try")) {
                responseBody << "Session value: " << session->value("try");
            } else {
                std::ostringstream val;
                val << time(NULL);
                session->setValue("try", val.str());
                responseBody << "Session value set to: " << session->value("try");
            }

            return res->send(responseBody.str());
        }

        bool forbid(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            res->setCode(HTTP_FORBIDDEN);
            return res->send("403 forbidden demo");
        }

        bool exception(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            throw std::string("Exception example");
        }

        bool uploadForm(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            std::stringstream responseBody;
            responseBody << "<html>";
            responseBody << "<h1>File upload demo (don't forget to create a tmp/ directory)</h1>";
            responseBody << "<form enctype=\"multipart/form-data\" method=\"post\">";
            responseBody << "Choose a file to upload: <input name=\"file\" type=\"file\" /><br />";
            responseBody << "<input type=\"hidden\" name=\"my fancy variable name\" value=\"my fancy variable value\" />";
            responseBody << "<input type=\"submit\" value=\"Upload File\" />";
            responseBody << "</form>";
            responseBody << "</fhtml>";

            return res->sendHtml(responseBody.str());
        }

        bool upload(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
        {
            std::stringstream responseBody;
            responseBody << "Your form variables: " << std::endl;

            for (const auto& variable: req->variables())
            {
                responseBody << variable.first << " : " << variable.second << std::endl;
            }

            return res->send(responseBody.str());
        }

        void setup()
        {
            // Hello demo
            addRoute("GET", "/hello", MyController, hello);
            addRoute("GET", "/hello_delayed", MyController, hello_delayed);
            addRoute("GET", "/", MyController, hello);

            // Form demo
            addRoute("GET", "/form", MyController, form);
            addRoute("POST", "/form", MyController, formPost);

            // Session demo
            addRoute("GET", "/session", MyController, session);

            // Exception example
            addRoute("GET", "/exception", MyController, exception);

            // 403 demo
            addRoute("GET", "/403", MyController, forbid);

            // File upload demo
            addRoute("GET", "/upload", MyController, uploadForm);
            addRoute("POST", "/upload", MyController, upload);

            //Generic register route
            registerRoute("GET", "/hello_lambda", [=](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
            {
                res->send("Hello lambda " + req->getVariable("name", "... what's your name ?") + "\n");
                return true;
            });

#ifdef HAS_JSON11
            //Generic register route
            registerRoute("GET", "/json", [=](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
            {
                json11::Json body = json11::Json::object {
                    {"hello", "world"},
                    {"status", 5}
                };
                res->sendJson(body);
                return true;
            });
#endif
        }
};

volatile static bool running = false;

void handle_signal(int sig)
{
    if (running)
    {
        std::cout << "Exiting..." << std::endl;
        running = false;
    }
}

int main()
{
    srand(Utils::getTime());
    signal(SIGINT, handle_signal);

    MyController myController;
    Server server("8080");
    server.registerController(&myController);
    server.setDirectoryListingEnabled(false);

    if (server.start())
    {
        std::cout << "Server started, routes:" << std::endl;
        myController.dumpRoutes();
        running = true;
    }
    

    while (running)
    {
        server.poll(1000);
    }
    
    return EXIT_SUCCESS;
}
