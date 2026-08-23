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
// File: SignalRCoreHandler.cpp
// Started by: Hattozo
// Started on: 8/19/2026
// Description: SignalR Core WebSocket transport used by recent voice-chat clients.
// (Disclaimer: Assisted by GPT 5.6 Sol)
#include <NoobWarrior/HttpServer/Emulator/SignalRCoreHandler.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/Registry.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/ws.h>

#include <algorithm>
#include <cctype>
#include <string>

using namespace NoobWarrior;

namespace {
struct SignalRConnection {
    evws_connection *WebSocket {nullptr};
    event *PingTimer {nullptr};
    bool HandshakeComplete {false};
    bool MessagePack {false};
    bool VerboseLogging {false};
};

void SendPing(SignalRConnection *connection) {
    if (connection->MessagePack) {
        // SignalR MessagePack framing: a two-byte payload ([6]) prefixed by its varint length.
        constexpr char ping[] = {0x02, static_cast<char>(0x91), 0x06};
        evws_send_binary(connection->WebSocket, ping, sizeof(ping));
    } else {
        const std::string ping = std::string(R"({"type":6})") + '\x1e';
        evws_send_text(connection->WebSocket, ping.c_str());
    }
}

void OnPingTimer(evutil_socket_t, short, void *arg) {
    SendPing(static_cast<SignalRConnection *>(arg));
}

void OnMessage(evws_connection *webSocket, int type, const unsigned char *data,
               size_t length, void *arg) {
    auto *connection = static_cast<SignalRConnection *>(arg);
    const std::string message(reinterpret_cast<const char *>(data), length);
    const bool isPing = connection->HandshakeComplete &&
        ((type == WS_TEXT_FRAME && message.find(R"("type":6)") != std::string::npos) ||
         (type == WS_BINARY_FRAME && length >= 3 && data[length - 1] == 0x06));

    if (!isPing || connection->VerboseLogging) {
        if (type == WS_TEXT_FRAME) {
            std::string printable = message.substr(0, 1024);
            std::replace_if(printable.begin(), printable.end(), [](const unsigned char ch) {
                return ch < 0x20 && ch != '\t';
            }, ' ');
            Out("SignalRCoreHandler", "Received SignalR text frame: {}", printable);
        } else {
            Out("SignalRCoreHandler", "Received SignalR binary frame ({} bytes)", length);
        }
    }

    if (!connection->HandshakeComplete) {
        // SignalR's handshake is always JSON text, even when subsequent hub messages use
        // MessagePack. The response is an empty JSON object terminated by the record separator.
        connection->MessagePack = message.find("messagepack") != std::string::npos;
        connection->HandshakeComplete = true;
        const std::string response = std::string("{}") + '\x1e';
        evws_send_text(webSocket, response.c_str());
        Out("SignalRCoreHandler", "Completed {} protocol handshake",
            connection->MessagePack ? "MessagePack" : "JSON");
        return;
    }

    // Client-to-server pings need no acknowledgement, but answering immediately keeps the
    // connection healthy even if Studio changes its timeout below our periodic interval.
    if (isPing) {
        SendPing(connection);
    }
}

void OnClose(evws_connection *, void *arg) {
    auto *connection = static_cast<SignalRConnection *>(arg);
    if (connection->PingTimer != nullptr) {
        event_del(connection->PingTimer);
        event_free(connection->PingTimer);
    }
    delete connection;
    Out("SignalRCoreHandler", "SignalR client disconnected");
}

void SendNegotiation(evhttp_request *req) {
    constexpr const char *body = R"({"negotiateVersion":1,"connectionId":"noobwarrior","connectionToken":"noobwarrior","availableTransports":[{"transport":"WebSockets","transferFormats":["Text","Binary"]}]})";
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer *reply = evbuffer_new();
    evbuffer_add(reply, body, std::char_traits<char>::length(body));
    evhttp_send_reply(req, HTTP_OK, nullptr, reply);
    evbuffer_free(reply);
}
}

SignalRCoreHandler::SignalRCoreHandler(Registry *registry) : mRegistry(registry) {}

void SignalRCoreHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char *uri = evhttp_request_get_uri(req);
    Out("SignalRCoreHandler", "Requested {}", uri ? uri : "");

    const char *upgrade = evhttp_find_header(evhttp_request_get_input_headers(req), "Upgrade");
    std::string upgradeValue = upgrade ? upgrade : "";
    std::transform(upgradeValue.begin(), upgradeValue.end(), upgradeValue.begin(),
                   [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (upgradeValue != "websocket") {
        SendNegotiation(req);
        return;
    }

    auto *connection = new SignalRConnection();
    connection->VerboseLogging = mRegistry != nullptr &&
        mRegistry->GetKeyValue<bool>("debug.log_voice_transport").value_or(false);
    connection->WebSocket = evws_new_session(req, OnMessage, connection, 0);
    if (connection->WebSocket == nullptr) {
        delete connection;
        Out("SignalRCoreHandler", "Failed to upgrade SignalR request to WebSocket");
        return;
    }

    evws_connection_set_closecb(connection->WebSocket, OnClose, connection);
    bufferevent *bufferEvent = evws_connection_get_bufferevent(connection->WebSocket);
    connection->PingTimer = event_new(bufferevent_get_base(bufferEvent), -1, EV_PERSIST,
                                      OnPingTimer, connection);
    if (connection->PingTimer != nullptr) {
        constexpr timeval interval {10, 0};
        event_add(connection->PingTimer, &interval);
    }
    Out("SignalRCoreHandler", "SignalR client connected");
}
