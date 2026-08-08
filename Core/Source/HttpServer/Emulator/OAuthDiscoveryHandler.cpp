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
// File: OAuthDiscoveryHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/OAuthDiscoveryHandler.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

OAuthDiscoveryHandler::OAuthDiscoveryHandler() {}

void OAuthDiscoveryHandler::OnRequest(evhttp_request *req, void *userdata) {
    nlohmann::json j;
    j["issuer"] = "https://apis.roblox.com";
    j["authorization_endpoint"] = "https://apis.roblox.com/oauth/v1/authorize";
    j["device_authorization_endpoint"] = "https://apis.roblox.com/oauth/v1/device/authorize";
    j["token_endpoint"] = "https://apis.roblox.com/oauth/v1/token";
    j["introspection_endpoint"] = "https://apis.roblox.com/oauth/v1/token/introspect";
    j["revocation_endpoint"] = "https://apis.roblox.com/oauth/v1/token/revoke";
    j["resources_endpoint"] = "https://apis.roblox.com/oauth/v1/token/resources";
    j["userinfo_endpoint"] = "https://apis.roblox.com/oauth/v1/userinfo";
    j["jwks_uri"] = "https://apis.roblox.com/oauth/v1/certs";
    j["registration_endpoint"] = "https://create.roblox.com/settings/api";
    j["service_documentation"] = "https://create.roblox.com/docs/cloud/auth/oauth2-overview";
    j["scopes_supported"] = nlohmann::json::array({"openid", "profile", "email", "phone", "address", "offline_access"});
    j["response_types_supported"] = nlohmann::json::array({"code"});
    j["response_modes_supported"] = nlohmann::json::array({"query"});
    j["token_endpoint_auth_methods_supported"] = nlohmann::json::array({"none", "client_secret_post", "client_secret_basic"});
    j["grant_types_supported"] = nlohmann::json::array({"authorization_code", "refresh_token", "urn:ietf:params:oauth:grant-type:device_code"});
    j["code_challenge_methods_supported"] = nlohmann::json::array({"S256"});
    j["subject_types_supported"] = nlohmann::json::array({"public"});
    j["id_token_signing_alg_values_supported"] = nlohmann::json::array({"ES256"});
    j["claims_supported"] = nlohmann::json::array({"sub", "name", "nickname", "preferred_username", "created_at", "profile", "picture"});
    j["claims_parameter_supported"] = false;
    j["request_parameter_supported"] = false;
    j["request_uri_parameter_supported"] = false;

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Set-Cookie",
        ".ROBLOSECURITY=_|WARNING:-DO-NOT-SHARE-THIS.--Sharing-this-will-allow-someone-to-log-into-your-account-and-to-steal-your-ROBUX-and-items.|_noobwarrior-studio-session; "
        "Domain=.roblox.com; Path=/; Secure; SameSite=None");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
