# Mongoose-C++

Mongoose-C++ is a C++ wrapper around the popular [mongoose](https://github.com/cesanta/mongoose/)
lightweight web server. This started off as a fork of https://github.com/Gregwar/mongoose-cpp. It is now moved to it's own repo, because mongoose-cpp itself is a fork of upstream mongoose - which we now use as vendor/mongoose submodule.

# Features

- Object-Oriented high level API, keeping the lightweight mongoose implementation
  as a backend
- Easy-to-use controllers sytem to build an application with modules
- Possibility of enabling Json11 to create a json compliant web application
- URL dispatcher using regex matches (C++11)
- Session system to store data about an user using cookies and garbage collect cleaning
- Simple access to GET & POST requests

# Hello world

Here is an example, this will serve the static files from `www/` directory (which
is the default setting) and the `/hello` page will be answered by a controller which
will display the GET `name` variable, for instance `/hello?name=bob` will display
the string "Hello bob". Default parameter value, if not provided, will be
"... waht's your name ?". This is the `helloworld` program build in the examples:

```c++
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
    bool hello(std::weak_ptr<Request> request, std::weak_ptr<Response> response)
    {
        auto req = request.lock();
        auto res = response.lock();
        assert(req);
        assert(res);

        std::string body =  "Hello " + req->getVariable("name", "... what's your name ?\n");
        return res->send(body);
    }

    void setup()
    {
        addRoute("GET", "/hello", MyController, hello);
        addRoute("GET", "/", MyController, hello);
    }
};


int main()
{
    MyController myController;
    Server server("8080");
    server.registerController(&myController);

    signal(SIGINT, handle_signal);

    server.start();

    while (true)
    {
        server.poll(1000);
    }

    return EXIT_SUCCESS;
}

```

# Building examples

You can build examples using CMake:

```
mkdir build
cd build
cmake -DEXAMPLES=ON ..
make
```

This will build you the `cpp` program with examples of GET, POST (form), session and 
HTTP response code

You can also enable Json example using the `-DHAS_JSON11=ON` option when cmake'ing,
this will build the `json` executable. You also have to specify the `JSON11_DIR` that is the [Json11](https://github.com/dropbox/json11) installation directory.


To enable url regex matching dispatcher use `-DENABLE_REGEX_URL=ON` option.
Note that this depends on C++11.

# Development

We maintain a patched fork The upstream mongoose web server library is present as a submodule in vendor/mongoose.
It is patched to enable multithreaded operations on mongoose buffers

# License

This project is licensed under MIT license.
However, the original mongoose project license is different, have a look to the
`vendor/mongoose/LICENSE` file for more information.
