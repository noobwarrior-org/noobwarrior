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
// File: IdeToolboxHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description: Legacy Studio toolbox endpoints. /IDE/Toolbox/Items.aspx returns the old
//              {"TotalResults":N,"Results":[...]} JSON; /IDE/ClientToolbox.aspx returns the old HTML
//              toolbox web view. Both list assets straight out of the mounted databases.
#include <NoobWarrior/HttpServer/Emulator/IdeToolboxHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ToolboxAssetCategory.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

using namespace NoobWarrior;

// Returns a single (decoded) query parameter, or "" when absent.
static std::string GetQueryParam(const char* uri, const char* key) {
    std::string out;
    if (uri == nullptr) return out;
    evkeyvalq query;
    if (evhttp_parse_query(uri, &query) == 0) {
        if (const char* val = evhttp_find_header(&query, key))
            out = val;
        evhttp_clear_headers(&query);
    }
    return out;
}

// Absolute origin (http://host) derived from the request's Host header, so URLs handed back to
// Studio's content fetcher resolve to whichever address the engine connected to.
static std::string BaseUrl(evhttp_request* req) {
    const char* host = evhttp_find_header(evhttp_request_get_input_headers(req), "Host");
    return std::string("http://") + (host ? host : "localhost");
}

// .NET-style "M/D/YYYY h:MM:SS AM/PM" timestamp, matching what the legacy toolbox emitted.
static std::string UnixToDotNet(int64_t t) {
    std::time_t tt = t > 0 ? static_cast<std::time_t>(t) : static_cast<std::time_t>(1420070400); // 2015-01-01
    std::tm tmv{};
#if defined(_WIN32)
    gmtime_s(&tmv, &tt);
#else
    gmtime_r(&tt, &tmv);
#endif
    int hour12 = tmv.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = tmv.tm_hour < 12 ? "AM" : "PM";
    return std::format("{}/{}/{} {}:{:02}:{:02} {}",
        tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_year + 1900, hour12, tmv.tm_min, tmv.tm_sec, ampm);
}

static std::string HtmlEscape(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += c; break;
        }
    }
    return out;
}

IdeToolboxHandler::IdeToolboxHandler(EmuDbManager *dbm) : mEmuDbManager(dbm) {}

void IdeToolboxHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    std::string path = uri ? uri : "";
    if (auto q = path.find('?'); q != std::string::npos)
        path.resize(q);

    // Lowercase the path so the /IDE vs /ide casing doesn't matter for dispatch.
    std::string lower = path;
    for (char &c : lower)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (lower.find("clienttoolbox") != std::string::npos)
        HandleClientToolboxHtml(req);
    else
        HandleItemsJson(req);
}

void IdeToolboxHandler::HandleItemsJson(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);
    std::string category = GetQueryParam(uri, "category");

    // The legacy server returns an empty result set when no category is selected.
    if (category.empty()) {
        nlohmann::json empty;
        empty["TotalResults"] = 0;
        empty["Results"] = nlohmann::json::array();
        const std::string body = empty.dump();
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
        evbuffer* buf = evbuffer_new();
        evbuffer_add(buf, body.data(), body.size());
        evhttp_send_reply(req, HTTP_OK, nullptr, buf);
        evbuffer_free(buf);
        return;
    }

    Roblox::AssetType type = ToolboxCategoryToAssetType(category);
    std::string keyword = GetQueryParam(uri, "keyword");
    std::string base = BaseUrl(req);

    std::vector<int64_t> ids = mEmuDbManager->SearchAssetIds(type, keyword, 30, 0);

    nlohmann::json results = nlohmann::json::array();
    for (int64_t id : ids) {
        std::optional<EmuDb::AssetSummary> summary = mEmuDbManager->GetAssetSummary(id);

        nlohmann::json asset;
        nlohmann::json creator;
        asset["Id"] = id;
        if (summary.has_value()) {
            asset["Name"] = summary->Name;
            asset["TypeId"] = summary->Type;
            asset["Description"] = summary->Description;
            asset["Created"] = UnixToDotNet(summary->Created);
            asset["Updated"] = UnixToDotNet(summary->Updated);

            if (summary->GroupId.has_value()) {
                creator["Id"] = summary->GroupId.value();
                creator["Name"] = mEmuDbManager->GetItemName(ItemType::Group, summary->GroupId.value()).value_or("Group");
                creator["Type"] = 2;
            } else {
                int64_t userId = summary->UserId.value_or(1);
                creator["Id"] = userId;
                creator["Name"] = mEmuDbManager->GetItemName(ItemType::User, userId).value_or("Player");
                creator["Type"] = 1;
            }
        } else {
            asset["Name"] = std::to_string(id);
            asset["TypeId"] = 0;
            asset["Description"] = "";
            asset["Created"] = UnixToDotNet(0);
            asset["Updated"] = UnixToDotNet(0);
            creator["Id"] = 1;
            creator["Name"] = "Player";
            creator["Type"] = 1;
        }
        asset["AssetGenres"] = nlohmann::json::array();
        asset["IsEndorsed"] = false;

        nlohmann::json thumbnail;
        thumbnail["Final"] = true;
        thumbnail["Url"] = base + "/asset?id=" + std::to_string(id);
        thumbnail["RetryUrl"] = nullptr;
        thumbnail["UserId"] = 0;
        thumbnail["EndpointType"] = "Avatar";

        nlohmann::json voting;
        voting["ShowVotes"] = true;
        voting["UpVotes"] = 0;
        voting["DownVotes"] = 0;
        voting["CanVote"] = false;
        voting["UserVote"] = nullptr;
        voting["HasVoted"] = false;
        voting["ReasonForNotVoteable"] = "InvalidAssetOrUser";

        nlohmann::json item;
        item["Asset"] = std::move(asset);
        item["Creator"] = std::move(creator);
        item["Thumbnail"] = std::move(thumbnail);
        item["Voting"] = std::move(voting);
        results.push_back(std::move(item));
    }

    nlohmann::json j;
    j["TotalResults"] = static_cast<int>(results.size());
    j["Results"] = std::move(results);

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}

void IdeToolboxHandler::HandleClientToolboxHtml(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);
    std::string category = GetQueryParam(uri, "category");
    if (category.empty())
        category = "FreeModels";

    Roblox::AssetType type = ToolboxCategoryToAssetType(category);
    std::string base = BaseUrl(req);
    std::vector<int64_t> ids = mEmuDbManager->SearchAssetIds(type, "", 30, 0);

    // The categories offered by the legacy toolbox dropdown that we can actually back with data.
    static const std::pair<const char*, const char*> kCategories[] = {
        {"FreeModels", "Free Models"},
        {"FreeDecals", "Free Decals"},
        {"Audio", "Audio"},
    };

    std::string options;
    for (const auto& [value, label] : kCategories) {
        options += std::format("<option value=\"{}\"{}>{}</option>",
            value, category == value ? " selected=\"selected\"" : "", label);
    }

    std::string items;
    for (int64_t id : ids) {
        std::optional<EmuDb::AssetSummary> summary = mEmuDbManager->GetAssetSummary(id);
        std::string name = HtmlEscape(summary.has_value() ? summary->Name : std::to_string(id));
        std::string thumb = base + "/asset?id=" + std::to_string(id);
        std::string insert = base + "/asset?id=" + std::to_string(id);
        items += std::format(
            "<span class=\"ToolboxItem\" ondragstart=\"dragRBX('{0}')\">"
            "<a title=\"{1}\" href=\"javascript:insertContent('{0}')\" "
            "style=\"display:inline-block;height:62px;width:60px;cursor:pointer;\">"
            "<img src=\"{2}\" border=\"0\" alt=\"{1}\"></a></span>",
            insert, name, thumb);
    }

    std::string html = std::format(
        "<!DOCTYPE html>\n<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">"
        "<title>Toolbox</title>"
        "<script type=\"text/javascript\">\n"
        "function insertContent(id){{ try{{ window.external.Insert(id); }}catch(x){{ alert('Could not insert the requested item'); }} }}\n"
        "function dragRBX(id){{ try{{ window.external.StartDrag(id); }}catch(x){{ }} }}\n"
        "</script></head><body class=\"Page\">"
        "<div id=\"ToolboxContainer\"><div id=\"ToolboxControls\"><div id=\"ToolboxSelector\">"
        "<select id=\"ddlToolboxes\" onchange=\"window.location='/IDE/ClientToolbox.aspx?category='+this.value;\">{0}</select>"
        "</div></div><div id=\"ToolboxItems\"><span id=\"dlToolboxItems\" style=\"display:inline-block;width:100%;\">{1}</span></div>"
        "</div></body></html>",
        options, items);

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=UTF-8");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, html.data(), html.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
