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
// File: StudioEditHandler.cpp
// Started by: Hattozo
// Started on: 3/22/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/StudioEditHandler.h>
#include <NoobWarrior/Log.h>

static constexpr const char* lol = R"(--rbxsig%DPs1HxWCagA8POZ7k2EDIlc1u8OjTRR+jAoomcPSQ7LxuX+lOXeqcWa5FiJnSJt5+L9jyuNdKcH7PFWUgCxQZKhYfHFQ5s+o2Q9KxCTsgZyIRgfSyPGMlo14P7j9uCvY1SmUJ8LWZKFYBaBaT82cTnm3wPLuuZPP9fnqYLgtfj0=%
-- Prepended to Edit.lua and Visit.lua and Studio.lua--

pcall(function() game:SetPlaceID(1818) end)

local message = Instance.new("Message")
message.Parent = workspace
message.archivable = false

game:GetService("ScriptInformationProvider"):SetAssetUrl("http://www.roblox.com/Asset/")
game:GetService("ContentProvider"):SetThreadPool(16)
pcall(function() game:GetService("InsertService"):SetFreeModelUrl("http://www.roblox.com/Game/Tools/InsertAsset.ashx?type=fm&q=%s&pg=%d&rs=%d") end) -- Used for free model search (insert tool)
pcall(function() game:GetService("InsertService"):SetFreeDecalUrl("http://www.roblox.com/Game/Tools/InsertAsset.ashx?type=fd&q=%s&pg=%d&rs=%d") end) -- Used for free decal search (insert tool)

settings().Diagnostics:LegacyScriptMode()

game:GetService("InsertService"):SetBaseSetsUrl("http://www.roblox.com/Game/Tools/InsertAsset.ashx?nsets=10&type=base")
game:GetService("InsertService"):SetUserSetsUrl("http://www.roblox.com/Game/Tools/InsertAsset.ashx?nsets=20&type=user&userid=%d")
game:GetService("InsertService"):SetCollectionUrl("http://www.roblox.com/Game/Tools/InsertAsset.ashx?sid=%d")
game:GetService("InsertService"):SetAssetUrl("http://www.roblox.com/Asset/?id=%d")
game:GetService("InsertService"):SetAssetVersionUrl("http://www.roblox.com/Asset/?assetversionid=%d")

pcall(function() game:GetService("SocialService"):SetFriendUrl("http://www.roblox.com/Game/LuaWebService/HandleSocialRequest.ashx?method=IsFriendsWith&playerid=%d&userid=%d") end)
pcall(function() game:GetService("SocialService"):SetBestFriendUrl("http://www.roblox.com/Game/LuaWebService/HandleSocialRequest.ashx?method=IsBestFriendsWith&playerid=%d&userid=%d") end)
pcall(function() game:GetService("SocialService"):SetGroupUrl("http://www.roblox.com/Game/LuaWebService/HandleSocialRequest.ashx?method=IsInGroup&playerid=%d&groupid=%d") end)
pcall(function() game:GetService("SocialService"):SetGroupRankUrl("http://www.roblox.com/Game/LuaWebService/HandleSocialRequest.ashx?method=GetGroupRank&playerid=%d&groupid=%d") end)
pcall(function() game:GetService("SocialService"):SetGroupRoleUrl("http://www.roblox.com/Game/LuaWebService/HandleSocialRequest.ashx?method=GetGroupRole&playerid=%d&groupid=%d") end)
pcall(function() game:GetService("GamePassService"):SetPlayerHasPassUrl("http://www.roblox.com/Game/GamePass/GamePassHandler.ashx?Action=HasPass&UserID=%d&PassID=%d") end)
pcall(function() game:GetService("MarketplaceService"):SetProductInfoUrl("https://api.roblox.com/marketplace/productinfo?assetId=%d") end)
pcall(function() game:GetService("MarketplaceService"):SetDevProductInfoUrl("https://api.roblox.com/marketplace/productDetails?productId=%d") end)
pcall(function() game:GetService("MarketplaceService"):SetPlayerOwnsAssetUrl("https://api.roblox.com/ownership/hasasset?userId=%d&assetId=%d") end)
pcall(function() game:SetCreatorID(0, Enum.CreatorType.User) end)

pcall(function() settings().Rendering.EnableFRM = true end)
pcall(function() settings()["Task Scheduler"].PriorityMethod = Enum.PriorityMethod.AccumulatedError end)

pcall(function() game:SetScreenshotInfo("") end)
pcall(function() game:SetVideoInfo("") end)

message.Text = "Loading Place. Please wait..."
coroutine.yield()
game:Load("http://www.roblox.com/Asset/?id=1818")

message.Parent = nil

game:GetService("ChangeHistoryService"):SetEnabled(true))";

using namespace NoobWarrior;

StudioEditHandler::StudioEditHandler() {

}

void StudioEditHandler::OnRequest(evhttp_request *req, void *userdata) {
    Out("StudioEditHandler", "Sending lua file...");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");
    evbuffer *reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", lol);
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
