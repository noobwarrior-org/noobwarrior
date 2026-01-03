// === noobWarrior ===
// File: HttpServer.h
// Started by: Hattozo
// Started on: 3/10/2025
// Description:
#pragma once
#include "Handler.h"
#include "RootHandler.h"
#include "TestHandler.h"

#include <cstdint>
#include <filesystem>
#include <vector>
#include <queue>
#include <memory>
#include <utility>

#include <evhttp.h>
#include <nlohmann/json_fwd.hpp>

#define NOOBWARRIOR_SET_URI(uri, handler)
#define NOOBWARRIOR_LINK_URI_TO_TEMPLATE(uri, fileName) SetRequestHandler(uri, mWebHandler.get(), (void*)fileName);

namespace NoobWarrior { class Core; }
namespace NoobWarrior::HttpServer {
enum class RenderResponse {
    Failed,
    Success,
    FailedRenderingBody,
    FailedRenderingMain,
    FailedOpeningTemplateFile
};

class HttpServer {
// TODO: The comments in this class are really shit and explains the properties really poorly.
// Someone needs to word it better than I can
    friend class RootHandler;
    friend class WebHandler;
public:
    HttpServer(Core *core, std::string logName = "HttpServer", std::string name = "");
    virtual ~HttpServer();
    
    virtual int Start(uint16_t port);
    virtual int Stop();

    bool        IsRunning();
    void        SetRequestHandler(const char *uri, Handler *handler, void *userdata = nullptr);

    /**
     * @brief Gets an absolute file path from a simple URI like "/img/icon1024.png".
     * It will check multiple directories for this resource, like the "common" folder
     * 
     * @param relativeFilePath The path to the file relative to the folder of the http server's assets
     * @param secondDirName The name of the folder inside of the http server's assets, which can either be "static" or "templates"
     * @return std::filesystem::path 
     */
    std::filesystem::path GetFilePath(std::string relativeFilePath, const std::string &secondDirName = "static");

    /**
     * @brief Renders a page using Jinja.
     * 
     * @param pageLoc 
     * @param data The JSON object that you want to pass onto the Jinja renderer.
     * @param output 
     * @return RenderResponse 
     */
    RenderResponse RenderPage(const std::string &pageLoc, nlohmann::json data, std::string *output);

    /**
     * @brief When you need to render HTML pages using Jinja, and need to give it information like the website branding.
     * Note that this contains the word "Base" in the title, so this is a bare minimum baseline that can be used to
     * render a blank page containing the website topbar without erroring.
     * Ideally, handlers should use their own method called GetContextData() which initially calls this.
     * 
     * @param conn An opaque handle representing a CivetWeb connection to a client. This can be nullable.
     * @return Returns a JSON object
     */
    virtual nlohmann::json GetBaseContextData(evhttp_request *req = nullptr);

    Core *GetCore();
protected:
    bool Running;

    // This is used in logging.
    std::string LogName;

    // Internal name of the server itself, used for locating the folder that this specific http server's assets are located in.
    // So if this is a server emulator, this should named "emulator" in order to find the appropriate folder.
    std::string Name;

    // This is supposed to be the baseline for where all the http server folders are located.
    // In it should contain folders like "common", "emulator", "meta", etc.
    // It's just in case a file isn't found in your folder specified in DirName, it has a baseline to fallback on.
    std::filesystem::path Directory;
    
    Core *mCore;
    
    evhttp* Server;

    //////////////// Handlers ////////////////
    std::unique_ptr<RootHandler> mRootHandler;
    std::unique_ptr<TestHandler> mTestHandler;

    std::vector<std::unique_ptr<std::pair<Handler*, void*>>> HandlerUserdata;
};
}