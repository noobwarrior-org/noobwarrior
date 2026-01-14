/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
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
            std::string failedMsg = "Failed to render page\n";
            switch (res) {
            default:
                failedMsg =+ "The reason is unknown.";
                break;
            case RenderResponse::FailedOpeningTemplateFile:
                failedMsg += "The template file failed to open.";
                break;
            }
            evhttp_send_error(req, HTTP_INTERNAL, failedMsg.c_str());

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