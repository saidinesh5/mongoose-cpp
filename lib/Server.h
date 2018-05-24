#ifndef _MONGOOSE_SERVER_H
#define _MONGOOSE_SERVER_H

#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <mongoose.h>


/**
 * Wrapper for the Mongoose server
 */
namespace Mongoose
{
class Controller;
class Request;
class Response;
class Server
{
public:
    /**
     * @brief Constructs the Server
     * @param bindAddress something like ":80", "0.0.0.0:80",  etc...
     * @param documentRoot Path to serve files from.
     */
    Server(const char *bindAddress = ":80", const char *documentRoot = "www");
    virtual ~Server();

    /**
     * @brief isRunning
     * @return true if the server is running
     */
    bool isRunning() const;

    /**
     * @brief start the server if it is already not started
     * @return true if the server is started
     */
    bool start();

    /**
     * @brief poll the server for incoming connections/requests for duration
     * @param duration - the number of milliseconds to poll the server for
     */
    void poll(int duration);

    /**
     * @brief stops the server if it is already running
     */
    void stop();

    /**
     * @brief registerController - add another controller that provides custom http routes
     */
    void registerController(Controller *c);

    /**
     * @brief deregisterController - removes the controller from all processing
     * @param c
     */
    void deregisterController(Controller *c);

    /**
     * @brief printStats prints basic statistics about the server to stdout
     */
    void printStats();

    /**
     * @brief handles
     * @param method
     * @param url
     * @return true if the seerver handles a URL
     */
    bool handles(const std::string& method, const std::string& url);

    bool allowMultipleClients() const;
    void setAllowMultipleClients(bool value);

    size_t uploadSizeLimit() const;
    void setUploadFileSizeLimit(size_t limit);

    std::string bindAddress() const;
    void setBindAddress(int address);

    bool directoryListingEnabled() const;
    void setDirectoryListingEnabled(bool value);

    std::string documentRoot() const;
    void setDocumentRoot(const std::string& root);

    std::string indexFiles() const;
    void setIndexFiles(const std::string& files);

    std::string authDomain() const;
    void setAuthDomain(const std::string& domain);

    std::string basicAuthUsername() const;
    void setBasicAuthUsername(const std::string& user);

    std::string basicAuthPassword() const;
    void setBasicAuthPassword(std::string password);

    bool requiresBasicAuthentication() const;

    std::string ipAccessControlList() const;
    void setIpAccessControlList(const std::string& acl);

    std::string hiddenFilePattern() const;
    void setHiddenFilePattern(const std::string& pattern);

    std::string extraHeaders() const;
    void setExtraHeaders(std::string headers);

    std::string tmpDir() const;
    void setTmpDir(const std::string& tmpDir);

private:
    static void ev_handler(struct mg_connection *c, int ev, void *p, void* ud);

    bool handleRequest(std::weak_ptr<Request> request, std::weak_ptr<Response> response);

    void updateHttpOptions();

    bool mIsRunning;
    struct mg_mgr mManager;
    struct mg_connection *mConnection{nullptr};
    struct mg_serve_http_opts mHttpOptions;

    //Internals
    std::map<struct mg_connection*, std::shared_ptr<Request>> mCurrentRequests;
    std::map<struct mg_connection*, std::shared_ptr<Response>> mCurrentResponses;
    std::vector<Controller *> mControllers;

    // Bind options
    std::string mBindAddress;
    bool mAllowMultipleClients;

    // Http Options
    std::string mDocumentRoot;
    std::string mIndexFiles;
    std::string mEnableDirectoryListing;
    std::string mAuthDomain;
    std::string mBasicAuthUsername;
    std::string mBasicAuthPassword;
    std::string mIpAccessControlList;
    std::string mHiddenFilePattern;
    std::string mExtraHeaders;
    size_t mUploadSizeLimit;

    // Statistics
    int mRequests{0};
    int mStartTime{0};

    std::string mTmpDir;
};
}

#endif
