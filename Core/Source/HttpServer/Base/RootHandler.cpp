// === noobWarrior ===
// File: RootHandler.cpp
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/HttpServer/Base/RootHandler.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>

#include <nlohmann/json.hpp>
#include <inja/inja.hpp>

#include <fstream>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

RootHandler::RootHandler(HttpServer *server) : mServer(server) {
    
}

void RootHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    std::filesystem::path file_path = mServer->GetFilePath(uri);
    if (file_path.empty()) {
        std::string pageName;
        std::string pageOutput;

        // Page should either be a 404 if the user intentionally tried to load in a non-empty URL that doesnt exist
        // But if its just the root point and nothing more, follow standard convention and load the home page.
        bool is_homepage = *uri == '/' && *(uri + 1) == '\0';
        pageName = is_homepage ? "home.jinja" : "not_found.jinja";
        RenderResponse res = mServer->RenderPage(pageName, mServer->GetBaseContextData(req), &pageOutput);

        if (res != RenderResponse::Success) {
            evhttp_send_error(req, HTTP_INTERNAL, "Failed to render page");
            return;
        }

        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html");

        evbuffer *reply = evbuffer_new();
        evbuffer_add_printf(reply, "%s", pageOutput.c_str());
        
        evhttp_send_reply(req, is_homepage ? HTTP_OK : HTTP_NOTFOUND, NULL, reply);

        evbuffer_free(reply);
    }
    /* TODO: add file support
#if !defined(_WIN32)
    mg_send_mime_file(req, file_path.c_str(), nullptr);
#else
    mg_send_mime_file(conn, WideCharToUTF8(const_cast<wchar_t*>(file_path.c_str())).c_str(), nullptr);
#endif
    */
    evhttp_send_reply(req, HTTP_OK, NULL, NULL);
}