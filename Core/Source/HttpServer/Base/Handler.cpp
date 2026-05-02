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
// File: Handler.cpp
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#include <NoobWarrior/HttpServer/Base/Handler.h>

#include <sstream>

using namespace NoobWarrior;

std::map<std::string, std::string> Handler::GetPostFormParameters(evhttp_request *req) {
    std::map<std::string, std::string> params;
    evhttp_cmd_type method = evhttp_request_get_command(req);
    if (method == EVHTTP_REQ_POST) {
        struct evbuffer *buf = evhttp_request_get_input_buffer(req);
        size_t len = evbuffer_get_length(buf);

        char *data = new char[len + 1];
        evbuffer_copyout(buf, data, len);
        data[len] = '\0';

        std::istringstream dataStream(data);
        std::string param;
        while (std::getline(dataStream, param, '&')) {
            std::string::size_type equal_pos = param.find('=');
            if (equal_pos != std::string::npos) {
                std::string key = param.substr(0, equal_pos);
                std::string val = param.substr(equal_pos + 1, std::string::npos);
                params[key] = val;
            }
        }

        delete[] data;
    }
    return params;
}
