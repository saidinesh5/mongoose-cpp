#include <string>
#include <iostream>
#include <algorithm>

#include <mongoose.h>

#include "Controller.h"
#include "Request.h"
#include "Response.h"
#include "Server.h"
#include "Utils.h"

using namespace std;
using namespace Mongoose;


namespace Mongoose
{

struct MultipartData
{
    FILE *currentFilePointer{nullptr};
    size_t currentEntityBytesWritten{0};
    std::string currentVariableData;
    std::vector<Request::MultipartEntity> multipartEntities;
};

void sendErrorNow(struct mg_connection* c, int errorCode, const char* errorString)
{
    mg_printf(c, "HTTP/1.0 %d %s\r\nContent-Length: 0\r\n\r\n", errorCode, errorString);
    c->flags |= MG_F_SEND_AND_CLOSE;
}

static struct mg_serve_http_opts sHttpOptions = {0};

/**
 * @brief Server::ev_handler
 * The main event handler - takes an incoming event
 * and creates a http Request/Response pair from it and hands it off to Server.
 * Upon a disocnnect event, it also deletes the Request/Response pair
 *
 */
void Server::ev_handler(struct mg_connection *c, int ev, void *p, void *ud)
{
    Server  *server = static_cast<Server*>(c->mgr->user_data);
    assert(server);

    if (server->requiresBasicAuthentication())
    {
        if(ev == MG_EV_HTTP_REQUEST
           || ev == MG_EV_HTTP_MULTIPART_REQUEST
           || ev == MG_EV_HTTP_PART_BEGIN
           || ev == MG_EV_HTTP_PART_DATA
           || ev == MG_EV_HTTP_PART_END
           || ev == MG_EV_HTTP_MULTIPART_REQUEST_END)
        {
            char username[256] = {0};
            char password[256] = {0};
            if (mg_get_http_basic_auth((struct http_message *) p,
                                       username, 256,
                                       password, 256) < 0
                || server->basicAuthUsername() != username
                || server->basicAuthPassword() != password)
            {
                mg_printf(c,
                          "HTTP/1.0 401 Unauthorized\r\n"
                          "WWW-Authenticate: Basic realm=\"%s\"\r\n"
                          "Content-Length: 0\r\n\r\n",
                          server->authDomain().c_str());
                c->flags |= MG_F_SEND_AND_CLOSE;
                return;
            }
        }
    }

    switch (ev)
    {
    case MG_EV_HTTP_REQUEST:
    {
        struct http_message *hm = (struct http_message *) p;
        c->user_data = NULL;

        //If server handles this request , let it.
        if (server->handles(std::string(hm->method.p, hm->method.len), std::string(hm->uri.p, hm->uri.len)))
        {
            auto request = std::make_shared<Request>(c, hm);
            auto response = std::make_shared<Response>(c);

            server->mCurrentRequests[c] = request;
            server->mCurrentResponses[c] = response;
            server->handleRequest(request, response);
        }
        else
        {
            //Else, simply let mongoose handle it - like a normal http server
            mg_serve_http(c, (struct http_message *) p, sHttpOptions);
        }

        break;
    }
    case MG_EV_HTTP_MULTIPART_REQUEST:
    {
        struct http_message *hm = (struct http_message *) p;

        if (server->handles(std::string(hm->method.p, hm->method.len), std::string(hm->uri.p, hm->uri.len)))
        {
            //Create a request/response pair now, because hm won't be available when we get
            //MG_EV_HTTP_MULTIPART_REQUEST
            auto request = std::make_shared<Request>(c, hm, true);
            auto response = std::make_shared<Response>(c);
            server->mCurrentRequests[c] = request;
            server->mCurrentResponses[c] = response;

            c->user_data = new MultipartData();
        }
        else
        {
            mg_printf(c, "%s",
                      "HTTP/1.0 404 Path not found\r\n"
                      "Content-Length: 0\r\n\r\n");
            c->user_data = NULL;
            c->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }

        break;
    }
    case MG_EV_HTTP_PART_BEGIN:
    {
        struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
        struct MultipartData *data = (MultipartData*)c->user_data;

        if (data != NULL)
        {
            data->currentEntityBytesWritten = 0;
            data->currentVariableData = "";

            if (std::string(mp->file_name).size() > 0)
            {
                std::string tmpFile = server->tmpDir() + "/" + Utils::sanitizeFilename(mp->file_name);
                data->currentFilePointer = fopen(tmpFile.c_str(), "wb");
                if (data->currentFilePointer == NULL)
                {
                    sendErrorNow(c, 500, "Failed to open a file");
                    delete data;
                    return;
                }
            }
        }
        else
        {
            sendErrorNow(c, 500, "Internal Server Error");
            return;
        }

        break;
    }
    case MG_EV_HTTP_PART_DATA:
    {
        struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
        struct MultipartData *data = (MultipartData*)c->user_data;

        if (data  != NULL)
        {
            if (server->uploadSizeLimit() < data->currentEntityBytesWritten + mp->data.len)
            {
                sendErrorNow(c, 413, "Requested Entity Too Large");
                return;
            }

            //If the uploaded data is a file, write it to a file.
            if (std::string(mp->file_name).size() > 0)
            {
                if (fwrite(mp->data.p, 1, mp->data.len, data->currentFilePointer) != mp->data.len)
                {
                    sendErrorNow(c, 500, "Failed to write a file");
                    return;
                }
            }
            else
            {
                data->currentVariableData += std::string(mp->data.p, mp->data.len);
            }

            data->currentEntityBytesWritten += mp->data.len;
        }
        else
        {
            sendErrorNow(c, 500, "Internal Server Error");
            c->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }

        break;
    }
    case MG_EV_HTTP_PART_END:
    {
        struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
        struct MultipartData *data = (MultipartData*)c->user_data;

        if (data != NULL)
        {
            data->multipartEntities.push_back(Request::MultipartEntity());
            Request::MultipartEntity& entity = data->multipartEntities.back();
            entity.fileName = mp->file_name;
            entity.variableName = mp->var_name;

            if (std::string(mp->file_name).size() > 0)
            {
                entity.filePath = server->tmpDir() + "/" + Utils::sanitizeFilename(entity.fileName);
                fclose(data->currentFilePointer);
            }
            else
            {
                entity.variableData = data->currentVariableData;
            }
        }
        else
        {
            sendErrorNow(c, 500, "Internal Server Error");
            return;
        }


        break;
    }
    case MG_EV_HTTP_MULTIPART_REQUEST_END:
    {
        struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
        struct MultipartData *data = (MultipartData*)c->user_data;

        if (data != NULL)
        {
            auto request = server->mCurrentRequests[c];
            auto response = server->mCurrentResponses[c];
            assert(request);
            assert(response);

            //Argument: mg_http_multipart_part, var_name and file_name are NULL,
            //status = 0 means request was properly closed, < 0 means connection was terminated
            if (mp->status == 0
                && server->handles(request->method(), request->url()))
            {
                request->setMultipartEntities(data->multipartEntities);
                server->handleRequest(request, response);
            }

            if (data != NULL)
            {
                delete data;
                c->user_data = NULL;
            }
        }
        else
        {
            sendErrorNow(c, 500, "Internal Server Error");
            return;
        }
        break;
    }
    case MG_EV_CLOSE:
    {
        if (server->mCurrentRequests.find(c) != server->mCurrentRequests.end())
        {
            server->mCurrentRequests[c]->setIsValid(false);
            server->mCurrentRequests.erase(c);
        }

        if (server->mCurrentResponses.find(c) != server->mCurrentResponses.end())
        {
            //To make sure any current mCurrentResponse.send() will fail
            server->mCurrentResponses[c]->setIsValid(false);
            server->mCurrentResponses.erase(c);
        }
        break;
    }
    }
}

Server::Server(const char *address, const char *documentRoot):
    mIsRunning(false),
    mUploadSizeLimit(1024*1024*100),
    mTmpDir("/tmp")
{
    memset(&sHttpOptions, 0, sizeof(sHttpOptions));
    setBindAddress(address);
    setDocumentRoot(documentRoot);
    setIndexFiles("index.html");
    setDirectoryListingEnabled(true);
}

Server::~Server()
{
    stop();
}

bool Server::start()
{

    if (!mIsRunning)
    {
        mManager = new (struct mg_mgr);
        mg_mgr_init(mManager, this);

        mRequests = 0;
        mStartTime = Utils::getTime();

        mConnection = mg_bind(mManager,
                              mBindAddress.c_str(),
                              ev_handler,
                              this);

        if (mConnection)
        {
            mg_set_protocol_http_websocket(mConnection);
            mIsRunning = true;
        }
    }

    if (mConnection == nullptr)
    {
        mg_mgr_free(mManager);
        mIsRunning = false;
        delete mManager;
        mManager = nullptr;
        std::cerr << "Error, unable to start server" << std::endl;
    }

    return mIsRunning;
}

void Server::poll(int duration)
{
    if (mIsRunning)
    {
        mg_mgr_poll(mManager, duration);
    }
}

void Server::stop()
{
    if (mIsRunning)
    {
        mg_mgr_free(mManager);
        delete mManager;
        mManager = nullptr;
        mIsRunning = false;
    }
}

void Server::registerController(Controller *controller)
{
    controller->setServer(this);
    controller->setup();
    mControllers.push_back(controller);
}

void Server::deregisterController(Controller *c)
{
    auto it = std::find(mControllers.begin(), mControllers.end(), c);

    if (it != mControllers.end())
    {
        mControllers.erase(it);
    }
}

bool Server::handleRequest(std::weak_ptr<Request> request, std::weak_ptr<Response> response)
{
    mRequests++;

    bool result = false;
    auto req = request.lock();
    auto res = response.lock();
    assert(req);
    assert(res);

    for (auto controller: mControllers)
    {
        if (controller->handles(req->method(), req->url()))
        {
            try
            {
                result = controller->handleRequest(request, response);
            }
            catch(...)
            {
                result = false;
            }

            if (res->isValid() && !result)
            {
                res->sendError("Server error trying to handle the request");
            }
            break;
        }
    }

    return result;
}

bool Server::handles(const string &method, const string &url)
{
    for (auto controller: mControllers)
    {
        if (controller->handles(method, url))
        {
            return true;
        }
    }

    return false;
}

bool Server::allowMultipleClients() const
{
    return mAllowMultipleClients;
}

void Server::setAllowMultipleClients(bool value)
{
    mAllowMultipleClients = value;
}

size_t Server::uploadSizeLimit() const
{
    return mUploadSizeLimit;
}

void Server::setUploadSizeLimit(size_t limit)
{
    mUploadSizeLimit = limit;
}

std::string Server::bindAddress() const
{
    return mBindAddress;
}

void Server::setBindAddress(const string &address)
{
    mBindAddress = address;
}

bool Server::directoryListingEnabled() const
{
    return mEnableDirectoryListing != "no";
}

void Server::setDirectoryListingEnabled(bool value)
{
    mEnableDirectoryListing = value? "yes" : "no";
    sHttpOptions.enable_directory_listing = mEnableDirectoryListing.c_str();
}

string Server::documentRoot() const
{
    return mDocumentRoot;
}

void Server::setDocumentRoot(const string &root)
{
    mDocumentRoot = root;
    sHttpOptions.document_root = mDocumentRoot.c_str();
}

string Server::indexFiles() const
{
    return mIndexFiles;
}

void Server::setIndexFiles(const string &files)
{
    mIndexFiles = files;
    sHttpOptions.index_files = mIndexFiles.c_str();
}

string Server::authDomain() const
{
    return mAuthDomain;
}

void Server::setAuthDomain(const string &domain)
{
    mAuthDomain = domain;
    sHttpOptions.auth_domain = mAuthDomain.c_str();
}

string Server::basicAuthUsername() const
{
    return mBasicAuthUsername;
}

void Server::setBasicAuthUsername(const string &user)
{
    mBasicAuthUsername = user;
}

string Server::basicAuthPassword() const
{
    return mBasicAuthPassword;
}

void Server::setBasicAuthPassword(const string &password)
{
    mBasicAuthPassword = password;
}

bool Server::requiresBasicAuthentication() const
{
    return mBasicAuthUsername.size() > 0 && mBasicAuthPassword.size() > 0;
}

string Server::ipAccessControlList() const
{
    return mIpAccessControlList;
}

void Server::setIpAccessControlList(const string &acl)
{
    mIpAccessControlList = acl;
    sHttpOptions.ip_acl = mIpAccessControlList.c_str();
}

string Server::hiddenFilePattern() const
{
    return mHiddenFilePattern;
}

void Server::setHiddenFilePattern(const string &pattern)
{
    mHiddenFilePattern = pattern;
    sHttpOptions.hidden_file_pattern = mHiddenFilePattern.c_str();
}

string Server::extraHeaders() const
{
    return mExtraHeaders;
}

void Server::setExtraHeaders(const string &headers)
{
    mExtraHeaders = headers;
}

string Server::tmpDir() const
{
    return mTmpDir;
}

void Server::setTmpDir(const string &tmpDir)
{
    mTmpDir = tmpDir;
}

void Server::printStats()
{
    int delta = Utils::getTime()-mStartTime;

    if (delta)
    {
        cout << "Requests: " << mRequests << ", Requests/s: " << (mRequests*1.0/delta) << endl;
    }
}
}
