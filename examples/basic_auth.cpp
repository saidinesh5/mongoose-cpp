#ifdef _MSC_VER
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <signal.h>
#include <cassert>

#include "Controller.h"
#include "Request.h"
#include "Response.h"
#include "Server.h"

using namespace std;
using namespace Mongoose;

class MyController : public Controller
{
public:
    bool hello(const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res)
    {
        std::string body;
        body = "Hello " + req->getVariable("name", "... what's your name ?\n");
        res->send(body);
        return true;
    }

    void setup()
    {
        addRoute("GET", "/hello", MyController, hello);
        addRoute("GET", "/", MyController, hello);
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
    MyController myController;
    Server server("8080");
    server.setBasicAuthUsername("admin");
    server.setBasicAuthPassword("admin");
    server.registerController(&myController);

    signal(SIGINT, handle_signal);

    if (server.start())
    {
        std::cout << "Server started, routes:" << std::endl;
        running = true;
    }


    while (running)
    {
        server.poll(1000);
    }

    return EXIT_SUCCESS;
}
