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
// File: Registry.cpp
// Started by: Hattozo
// Started on: 3/8/2025
// Description: Tweaks various parameters of noobWarrior functionality
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <fstream>
#include <nlohmann/json_fwd.hpp>
#include <unistd.h>

using namespace NoobWarrior;

Registry::Registry(const std::filesystem::path &filePath, LuaState* lua) : BaseRegistry("registry", filePath, lua)
{}

RegistryResponse Registry::Open() {
    if (const RegistryResponse res = BaseRegistry::Open(); res != RegistryResponse::Success) return res;
    SetKeyValue("meta.version", NOOBWARRIOR_REGISTRY_VERSION);
    SetKeyValueIfNotSet("language", "en_US");
    SetKeyValueIfNotSet("gui.theme", "default");

    sol::table master_servers_tbl = mLua->create_table();
    SetKeyValueIfNotSet("gui.master_servers", master_servers_tbl);

    sol::table databases_tbl = mLua->create_table();
    SetKeyValueIfNotSet("databases", databases_tbl);

    sol::table plugins_tbl = mLua->create_table();
    SetKeyValueIfNotSet("plugins", plugins_tbl);

    SetKeyValueIfNotSet("internet.roblox.asset_delivery", "https://assetdelivery.roblox.com/v1/asset/?id={}");
    SetKeyValueIfNotSet("internet.roblox.asset_details", "https://economy.roblox.com/v2/assets/{}/details");
    SetKeyValueIfNotSet("internet.roblox.badge_details", "https://badges.roblox.com/v1/badges/{}");
    SetKeyValueIfNotSet("internet.roblox.universe_details", "https://games.roblox.com/v1/games?universeIds={}");
    SetKeyValueIfNotSet("internet.roblox.universe_places", "https://develop.roblox.com/v1/universes/{}/places");
    SetKeyValueIfNotSet("internet.roblox.universe_badges", "https://badges.roblox.com/v1/universes/{}/badges");
    SetKeyValueIfNotSet("internet.roblox.user_details", "https://users.roblox.com/v1/users/{}");
    SetKeyValueIfNotSet("internet.roblox.group_details", "https://groups.roblox.com/v1/groups/{}");

    SetKeyValueIfNotSet("user.id", 1000);
    SetKeyValueIfNotSet("user.name", "Player");
    SetKeyValueIfNotSet("user.display_name", "Player");

    SetKeyValueIfNotSet("user.appearance.tshirt", 0);
    SetKeyValueIfNotSet("user.appearance.shirt", 0);
    SetKeyValueIfNotSet("user.appearance.pants", 0);
    SetKeyValueIfNotSet("user.appearance.face", 0);

    SetKeyValueIfNotSet("user.appearance.color.head", "Bright yellow");
    SetKeyValueIfNotSet("user.appearance.color.torso", "Bright blue");
    SetKeyValueIfNotSet("user.appearance.color.left_arm", "Bright yellow");
    SetKeyValueIfNotSet("user.appearance.color.right_arm", "Bright yellow");
    SetKeyValueIfNotSet("user.appearance.color.left_leg", "Br. yellowish green");
    SetKeyValueIfNotSet("user.appearance.color.right_leg", "Br. yellowish green");

    SetKeyValueIfNotSet("user.appearance.accessories", mLua->create_table());

    SetKeyValueIfNotSet("user.appearance.body.head", 0);
    SetKeyValueIfNotSet("user.appearance.body.torso", 0);
    SetKeyValueIfNotSet("user.appearance.body.left_arm", 0);
    SetKeyValueIfNotSet("user.appearance.body.right_arm", 0);
    SetKeyValueIfNotSet("user.appearance.body.left_leg", 0);
    SetKeyValueIfNotSet("user.appearance.body.right_leg", 0);

    SetKeyValueIfNotSet("user.appearance.animation.climb", 0);
    SetKeyValueIfNotSet("user.appearance.animation.fall", 0);
    SetKeyValueIfNotSet("user.appearance.animation.idle", 0);
    SetKeyValueIfNotSet("user.appearance.animation.jump", 0);
    SetKeyValueIfNotSet("user.appearance.animation.run", 0);
    SetKeyValueIfNotSet("user.appearance.animation.swim", 0);
    SetKeyValueIfNotSet("user.appearance.animation.walk", 0);

    SetKeyValueIfNotSet("user.appearance.scale.body_type", 0);
    SetKeyValueIfNotSet("user.appearance.scale.depth", 0);
    SetKeyValueIfNotSet("user.appearance.scale.head", 0);
    SetKeyValueIfNotSet("user.appearance.scale.height", 0);
    SetKeyValueIfNotSet("user.appearance.scale.proportion", 0);
    SetKeyValueIfNotSet("user.appearance.scale.width", 0);

    SetKeyValueIfNotSet("emu.branding.title", "noobWarrior Server Emulator");
    SetKeyValueIfNotSet("emu.branding.icon", "/img/icon1024.png");
    SetKeyValueIfNotSet("emu.branding.tagline", "My noobWarrior server");
    SetKeyComment("emu.branding", "The branding that people will see when they connect to your website.");

    SetKeyValueIfNotSet("emu.auth.type", "master");
    SetKeyComment("emu.auth.type", "If set to \"master\", your server is responsible for all authentication. If set to \"slave\", the server URL set in the \"master\" variable will be responsible for all authentication.");

    SetKeyValueIfNotSet("emu.auth.master", "");
    SetKeyComment("emu.auth.master", "The URL of the master server that your server's authentication system accepts. Does nothing if the auth type is set to \"master\"");

    SetKeyValueIfNotSet("emu.auth.allow_registration", false);
    SetKeyComment("emu.auth.allow_registration", "If this is set to false, registrations for guests will be disabled and administrators must manually create accounts in a database.");

    SetKeyValueIfNotSet("emu.auth.password_based", true);
    SetKeyComment("emu.auth.password_based", "If true, enables password-based authentication. If set to false, then the only way the user will be allowed to login is if they use OAuth2 based services.");

    SetKeyValueIfNotSet("emu.auth.enable_login_filter", false);
    SetKeyComment("emu.auth.enable_login_filter", "Makes it so that any users who are blacklisted or not whitelisted will not be able to log in.");

    SetKeyValueIfNotSet("emu.auth.login_filter_type", "whitelist");
    SetKeyComment("emu.auth.login_filter_type", "This setting only applies if enable_login_filter is set to true. You can set this to either \"blacklist\" or \"whitelist\"");

    SetKeyValueIfNotSet("emu.auth.enable_custom_pfp", false);
    SetKeyComment("emu.auth.enable_custom_pfp", "Allows users to upload their own profile pictures instead of using their avatar headshot.");

    SetKeyValueIfNotSet("emu.motd", "<h1>Welcome</h1><p>Welcome to my noobWarrior server.</p><h2>Rules</h2><p>The operator of this server has not set any rules. However, don't take this as an opportunity to be a jackass and instead have some common courtesy.</p>");

    SetKeyValueIfNotSet("emu.http_port", 8080);
    SetKeyComment("emu.http_port", "The port that the HTTP server emulator should listen on.");

    SetKeyValueIfNotSet("emu.https_port", 53640);
    SetKeyComment("emu.https_port", "The port that the HTTPS server emulator should listen on.");

    SetKeyValueIfNotSet("emu.asset_grab_mode", false);
    SetKeyComment("emu.asset_grab_mode", "If enabled, any asset that is retrieved from Roblox services will be downloaded and saved to a database of your choice.");

    SetKeyValueIfNotSet("emu.asset_grab_db", "");
    SetKeyComment("emu.asset_grab_db", "Where should the Asset Grab Mode save its assets to?");

    sol::table proxies_tbl = mLua->create_table();
    SetKeyValueIfNotSet("emu.proxies", proxies_tbl);
    SetKeyComment("emu.proxies", "Any proxies in this list will be used as a fallback reverse proxy for API requests in case yours fail.");

    SetKeyValueIfNotSet("emu.enable_roblox_proxy", true);
    SetKeyComment("emu.enable_roblox_proxy", "Use the Roblox API as a fallback reverse proxy for API requests. Note that this requires the program to be logged in to your Roblox account in order for it to work.");

    SetKeyValueIfNotSet("emu.game_view_mode", "server");
    SetKeyComment("emu.game_view_mode", "This value can be set to three modes, \"server\", \"game\", and \"place\". If set to \"server\", it will display individual servers on the website. If set to \"game\", it will display an entire Roblox game containing all the servers being hosted for it and the places it contains; this is how Roblox does it for their website. If set to \"place\", individual places that belong to a universe will be shown; this is how Roblox did it before introducing the \"Universe\" system in 2014.");

    SetKeyValueIfNotSet("emu.connect_to_individual_place", true);

    sol::table banners_tbl = mLua->create_table();
    SetKeyValueIfNotSet("emu.banners", banners_tbl);

    // SetKeyValueIfNotSet("emu.banner.background_color", "#ff8000");
    // SetKeyValueIfNotSet("emu.banner.foreground_color", "#ffffff");
    // SetKeyValueIfNotSet("emu.banner.message", "");
    // SetKeyComment("emu.banner", "Customizes the banner that any visitor who connects to your website will see.");

    SetKeyValueIfNotSet("emu.homepage.guest_msg", R"(<h1>Welcome</h1>
<p>This message appears when you try to access the homepage while logged out.</p>
<p>Please log in by clicking the Login button.</p>

<h2>For Server Operators</h2>
<p>If you are the operator of this server and are setting this up for the first time, you need to manually enable registration. You should also probably edit this message; you can do so by opening the emu.lua file in the registry directory and editing the "emu.homepage.guest_msg" key.</p>
<p>To administer the server emulator from this website, you need to create an admin account in the master database using either the SDK or the command-line interface. Whatever you should use depends on if you are hosting this on a server with a GUI or not.</p>)");

    if (!GetKeyValue<sol::table>("emu.roles").has_value()) {
        sol::table roles_tbl = mLua->create_table();
        roles_tbl[1] = mLua->create_table();
        roles_tbl[1]["name"] = "Guest";
        roles_tbl[1]["rank"] = 0;

        roles_tbl[2] = mLua->create_table();
        roles_tbl[2]["name"] = "User";
        roles_tbl[2]["rank"] = 1;

        roles_tbl[3] = mLua->create_table();
        roles_tbl[3]["name"] = "Moderator";
        roles_tbl[3]["rank"] = 100;

        roles_tbl[4] = mLua->create_table();
        roles_tbl[4]["name"] = "Administrator";
        roles_tbl[4]["rank"] = 200;

        roles_tbl[5] = mLua->create_table();
        roles_tbl[5]["name"] = "Operator";
        roles_tbl[5]["rank"] = 255;
        SetKeyValue("emu.roles", roles_tbl);
    }

    SetKeyValueIfNotSet("emu.permissions.ugc.catalog.clothing", 100);
    SetKeyComment("emu.permissions.ugc.catalog.clothing", "What rank is able to upload clothing to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.catalog.accessories", 100);
    SetKeyComment("emu.permissions.ugc.catalog.accessories", "What rank is able to upload accessories to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.catalog.heads", 100);
    SetKeyComment("emu.permissions.ugc.catalog.heads", "What rank is able to upload heads to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.catalog.bundles", 100);
    SetKeyComment("emu.permissions.ugc.catalog.bundles", "What rank is able to upload bundles to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.catalog.animations", 100);
    SetKeyComment("emu.permissions.ugc.catalog.animations", "What rank is able to upload avatar animations to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.catalog.emotes", 100);
    SetKeyComment("emu.permissions.ugc.catalog.emotes", "What rank is able to upload emotes to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.catalog.other", 100);
    SetKeyComment("emu.permissions.ugc.catalog.other", "Anything not listed in the emu.permissions.ugc.catalog list goes here.");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.games", 100);
    SetKeyComment("emu.permissions.ugc.dev.games", "What rank is able to upload games to the website? This includes universes and places.");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.badges", 1);
    SetKeyComment("emu.permissions.ugc.dev.badges", "What rank is able to upload badges to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.passes", 1);
    SetKeyComment("emu.permissions.ugc.dev.passes", "What rank is able to upload gamepasses to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.models", 100);
    SetKeyComment("emu.permissions.ugc.dev.models", "What rank is able to upload models to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.video", 100);
    SetKeyComment("emu.permissions.ugc.dev.video", "What rank is able to upload video to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.plugins", 100);
    SetKeyComment("emu.permissions.ugc.dev.plugins", "What rank is able to upload plugins to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.audio", 100);
    SetKeyComment("emu.permissions.ugc.dev.audio", "What rank is able to upload audio to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.animations", 100);
    SetKeyComment("emu.permissions.ugc.dev.animations", "What rank is able to upload game animations to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.meshes", 100);
    SetKeyComment("emu.permissions.ugc.dev.meshes", "What rank is able to upload meshes to the website?");

    SetKeyValueIfNotSet("emu.permissions.ugc.dev.other", 100);
    SetKeyComment("emu.permissions.ugc.dev.other", "Anything not listed in the emu.permissions.ugc.dev list goes here.");

    SetKeyValueIfNotSet("emu.permissions.access.home", 1);
    SetKeyComment("emu.permissions.access.servers", "What rank is able to access the Home page on the website?");

    SetKeyValueIfNotSet("emu.permissions.access.servers", 1);
    SetKeyComment("emu.permissions.access.servers", "What rank is able to access the Servers page on the website?");

    SetKeyValueIfNotSet("emu.permissions.access.library", 1);

    SetKeyValueIfNotSet("emu.permissions.access.control_panel", 200);

    SetKeyValueIfNotSet("emu.permissions.remote_start_server", 1);
    SetKeyComment("emu.permissions.remote_start_server", "What rank is able to remotely start game servers? Remotely starting game servers means being able to click \"Play\" on a game and having a server automatically launch for you if none exists.");

    SetKeyValueIfNotSet("emu.permissions.connect_to_server", 0);
    SetKeyComment("emu.permissions.connect_to_server", "What rank is able to connect to your server? By default, this is set to \"guest\" for convenience reasons.");

    SetKeyValueIfNotSet("emu.permissions.ban", 100);
    SetKeyComment("emu.permissions.ban", "What rank is able to ban users?");

    SetKeyValueIfNotSet("emu.currency.enabled", false);
    SetKeyComment("emu.currency.enabled", "If this is enabled, users will have a visible Robux balance on the website and will be able to spend it.");

    SetKeyValueIfNotSet("emu.currency.enable_tix", true);
    SetKeyComment("emu.currency.enable_tix", "Enables the ability for people to spend Tickets, a freemium currency on Roblox that was removed in 2016.");

    SetKeyValueIfNotSet("emu.currency.make_everything_free", true);
    SetKeyComment("emu.currency.make_everything_free", "If this is enabled, everything on the website will be free regardless of the price it is set to. Note that disabling this and keeping the currency feature disabled will make no one be able to buy anything.");

    SetKeyValueIfNotSet("emu.currency.starting_robux", 0);
    SetKeyComment("emu.currency.starting_robux", "If this is set to a number above 0, any new registrars will receive the specified amount of Robux. This does not apply retroactively.");

    SetKeyValueIfNotSet("emu.currency.starting_tix", 0);
    SetKeyComment("emu.currency.starting_tix", "If this is set to a number above 0, any new registrars will receive the specified amount of Tix. This does not apply retroactively.");

    SetKeyValueIfNotSet("emu.currency.daily_robux", 0);
    SetKeyValueIfNotSet("emu.currency.daily_tix", 0);

    SetKeyValueIfNotSet("wine.exe", "wine");
    SetKeyValueIfNotSet("wine.prefix", "");
    return RegistryResponse::Success;
}
