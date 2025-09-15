// === noobWarrior ===
// File: HttpServer.h
// Started by: Hattozo
// Started on: 3/10/2025
// Description:
#pragma once
#include "Handler.h"

#include <cstdint>
#include <filesystem>
#include <vector>
#include <queue>
#include <utility>

#define NOOBWARRIOR_SET_URI(uri, handler)
#define NOOBWARRIOR_LINK_URI_TO_TEMPLATE(uri, fileName) SetRequestHandler(uri, mWebHandler, (void*)fileName);

struct mg_context;
namespace NoobWarrior { class Core; }
namespace NoobWarrior::HttpServer {
    class RootHandler;
    class WebHandler;
    class HttpServer {
    // TODO: The comments in this class are really shit and explains the properties really poorly.
    // Someone needs to word it better than I can
        friend class RootHandler;
        friend class WebHandler;
    public:
        HttpServer(Core *core, std::string name = "HttpServer", std::string dirName = "");
        
        virtual int Start(uint16_t port);
        virtual int Stop();

        bool        IsRunning();
        void        SetRequestHandler(const char *uri, Handler *handler, void *userdata = nullptr);

        /**
         * @brief Get the File Path object
         * 
         * @param relativeFilePath The path to the file relative to the folder of the http server's assets
         * @param secondDirName The name of the folder inside of the http server's assets, which can either be "static" or "templates"
         * @return std::filesystem::path 
         */
        std::filesystem::path GetFilePath(std::string relativeFilePath, const std::string &secondDirName = "static");
    protected:
        bool Running;

        // This is used in logging.
        std::string Name;

        // This is supposed to be the name of the folder that this specific http server's assets are located in.
        // So if this is a server emulator, this should be the "emulator" folder and such.
        std::string DirName;

        // This is supposed to be the baseline for where all the http server folders are located.
        // In it should contain folders like "common", "emulator", "meta", etc.
        // It's just in case a file isn't found in your folder specified in DirName, it has a baseline to fallback on.
        std::filesystem::path Directory;
        
        Core *mCore;
        
        mg_context *Server;

        //////////////// Handlers ////////////////
        RootHandler *mRootHandler;
        WebHandler *mWebHandler;

        std::vector<void**> HandlerUserdata;
    };
}