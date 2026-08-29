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
#include <NoobWarrior/EmuDb/UserRank.h>
#include <NoobWarrior/Macros.h>

#include <cstdint>
#include <random>

using namespace NoobWarrior;

Registry::Registry(const std::filesystem::path &filePath, LuaState* lua) : BaseRegistry("registry", filePath, lua)
{}

RegistryResponse Registry::Open() {
    if (const RegistryResponse res = BaseRegistry::Open(); res != RegistryResponse::Success) return res;
    SetKeyValue("meta.version", NOOBWARRIOR_REGISTRY_VERSION);
    SetKeyValueIfNotSet("language", "en_US");
    SetKeyValueIfNotSet("gui.theme", "default");

    sol::table master_servers_tbl = mLua->create_table();
    SetKeyValueIfNotSet("master_servers", master_servers_tbl);

    sol::table databases_tbl = mLua->create_table();
    SetKeyValueIfNotSet("databases", databases_tbl);

    sol::table plugins_tbl = mLua->create_table();
    SetKeyValueIfNotSet("plugins", plugins_tbl);

    SetKeyValueIfNotSet("disable_rbxl_mutation", false);
    SetKeyComment("disable_rbxl_mutation", "If true, will disable " NOOBWARRIOR_BRAND "'s ability to edit place files on demand when loading plugin datamodels and scripts.");

    SetKeyValueIfNotSet("debug.log_http_server_requests", false);

    SetKeyValueIfNotSet("debug.log_voice_transport", false);
    SetKeyComment("debug.log_voice_transport", "If true, logs high-frequency voice transport diagnostics, including TURN/STUN packet traces and SignalR heartbeat frames. Leave disabled unless troubleshooting voice connectivity.");

    SetKeyValueIfNotSet("allow_multiple_instances", false);
    SetKeyComment("allow_multiple_instances", "If true, lets more than one copy of " NOOBWARRIOR_BRAND " run at once instead of blocking the second one. Only enable this if you know what you're doing!");

    SetKeyValueIfNotSet("internet.roblox.asset_delivery", "https://assetdelivery.roblox.com/v1/asset/?id={}");
    SetKeyValueIfNotSet("internet.roblox.asset_details", "https://economy.roblox.com/v2/assets/{}/details");
    SetKeyValueIfNotSet("internet.roblox.badge_details", "https://badges.roblox.com/v1/badges/{}");
    SetKeyValueIfNotSet("internet.roblox.universe_details", "https://games.roblox.com/v1/games?universeIds={}");
    SetKeyValueIfNotSet("internet.roblox.universe_places", "https://develop.roblox.com/v1/universes/{}/places");
    SetKeyValueIfNotSet("internet.roblox.universe_badges", "https://badges.roblox.com/v1/universes/{}/badges");
    SetKeyValueIfNotSet("internet.roblox.user_details", "https://users.roblox.com/v1/users/{}");
    SetKeyValueIfNotSet("internet.roblox.user_avatar_details", "https://avatar.roblox.com/v1/users/{}/avatar");
    SetKeyValueIfNotSet("internet.roblox.avatar_fetch", "https://avatar.roblox.com/v2/avatar/avatar-fetch?userId={}&placeId=1");
    SetKeyValueIfNotSet("internet.roblox.user_thumbnail", "https://thumbnails.roblox.com/v1/users/avatar-headshot?userIds={}&size=420x420&format=Png&isCircular=false");
    SetKeyValueIfNotSet("internet.roblox.user_avatar", "https://thumbnails.roblox.com/v1/users/avatar?userIds={}&size=420x420&format=Png&isCircular=false");
    SetKeyValueIfNotSet("internet.roblox.group_details", "https://groups.roblox.com/v1/groups/{}");
    SetKeyValueIfNotSet("internet.roblox.bundle_details", "https://catalog.roblox.com/v1/bundles/{}/details");
    SetKeyValueIfNotSet("internet.roblox.asset_thumbnail", "https://thumbnails.roblox.com/v1/assets?assetIds={}&size=420x420&format=Png&isCircular=false");
    SetKeyValueIfNotSet("internet.roblox.universe_thumbnails", "https://thumbnails.roblox.com/v1/games/multiget/thumbnails?universeIds={}&size=768x432&format=Png&countPerUniverse=25&defaults=true");
    SetKeyValueIfNotSet("internet.roblox.universe_icon", "https://thumbnails.roblox.com/v1/games/icons?universeIds={}&size=512x512&format=Png&isCircular=false");
    SetKeyValueIfNotSet("internet.roblox.place_universe", "https://apis.roblox.com/universes/v1/places/{}/universe");

    SetKeyValueIfNotSet("backup.max_depth", 6);
    SetKeyComment("backup.max_depth", "How many levels deep a backup follows related items. Higher captures more related items but is slower; it is clamped to a safe maximum (20) to avoid recursion/stack overflow.");

    SetKeyValueIfNotSet("sdk.recent_projects", mLua->create_table());
    SetKeyComment("sdk.recent_projects", "Paths of the projects most recently opened in the SDK, most recent first. The SDK's welcome page reads this list; deleting it just empties that list.");
    SetKeyValueIfNotSet("sdk.max_recent_projects", 10);
    SetKeyComment("sdk.max_recent_projects", "How many entries sdk.recent_projects keeps before the oldest is dropped.");

    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<int64_t> dist(1, 2147483646);
        SetKeyValueIfNotSet<int64_t>("user.id", dist(gen));
    }
    SetKeyValueIfNotSet("user.name", "Player");
    SetKeyValueIfNotSet("user.display_name", "Player");

    SetKeyValueIfNotSet("user.appearance.avatar_type", "R6"); // "R6" or "R15"

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

    SetKeyValueIfNotSet("user.appearance.body.package", 0);
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

    SetKeyValueIfNotSet("emu.branding.title", NOOBWARRIOR_BRAND " Server Emulator");
    SetKeyValueIfNotSet("emu.branding.icon", "/img/icon1024.png");
    SetKeyValueIfNotSet("emu.branding.tagline", "My " NOOBWARRIOR_BRAND " server");
    SetKeyComment("emu.branding", "The branding that people will see when they connect to your website.");

    SetKeyValueIfNotSet("emu.auth.enabled", false);
    SetKeyComment("emu.auth.enabled", "If set to true, requires people to authenticate to join your server.");

    SetKeyValueIfNotSet("emu.auth.type", "master");
    SetKeyComment("emu.auth.type", "If set to \"master\", your server is responsible for all authentication. If set to \"slave\", the server URL set in the \"master\" variable will be responsible for all authentication.");

    SetKeyValueIfNotSet("emu.auth.master", "");
    SetKeyComment("emu.auth.master", "The URL of the master server that your server's authentication system accepts. Does nothing if the auth type is set to \"master\"");

    SetKeyValueIfNotSet("emu.auth.federated_login", true);
    SetKeyComment("emu.auth.federated_login", "Slave mode only. If enabled, players logged in to any master server that is federated (and not defederated) with your master server may join. If disabled, only accounts on your own master server may join.");

    SetKeyValueIfNotSet("emu.master.announce", false);
    SetKeyComment("emu.master.announce", "If enabled, this server emulator will announce itself to the master server at emu.auth.master whenever it has running game servers, so it appears in that master server's public server list. It stops announcing (and is removed from the list) once it has no running game servers.");
    
    SetKeyValueIfNotSet("emu.auth.allow_registration", false);
    SetKeyComment("emu.auth.allow_registration", "If this is set to false, registrations for guests will be disabled and administrators must manually create accounts in a database.");

    SetKeyValueIfNotSet("emu.auth.password_based", true);
    SetKeyComment("emu.auth.password_based", "If true, enables password-based authentication. If set to false, then the only way the user will be allowed to login is if they use OAuth2 based services.");

    SetKeyValueIfNotSet("emu.auth.allow_accounts_from_all_mounted_databases", false);
    SetKeyComment("emu.auth.allow_accounts_from_all_mounted_databases", "If true, allows users in any mounted database to be logged into. This is a security risk if enabled because you could import databases from others and they'd be able to login to your server with admin permissions.");

    SetKeyValueIfNotSet("emu.auth.enable_login_filter", false);
    SetKeyComment("emu.auth.enable_login_filter", "Makes it so that any users who are blacklisted or not whitelisted will not be able to log in.");

    SetKeyValueIfNotSet("emu.auth.login_filter_type", "whitelist");
    SetKeyComment("emu.auth.login_filter_type", "This setting only applies if enable_login_filter is set to true. You can set this to either \"blacklist\" or \"whitelist\"");

    SetKeyValueIfNotSet("emu.auth.enable_custom_pfp", false);
    SetKeyComment("emu.auth.enable_custom_pfp", "Allows users to upload their own profile pictures instead of using their avatar headshot.");

    SetKeyValueIfNotSet("emu.auth.allow_guests", false);
    SetKeyComment("emu.auth.allow_guests", "Only applies when emu.auth.enabled is true. If enabled, players who are not logged in may still join as a Guest instead of being turned away. If disabled, joining requires a logged-in account.");

    SetKeyValueIfNotSet<int64_t>("emu.auth.ticket_ttl", 120);
    SetKeyComment("emu.auth.ticket_ttl", "How long, in seconds, a one-time game-join authentication ticket remains valid before it expires. The game server must redeem it within this window.");

    SetKeyValueIfNotSet<int64_t>("emu.auth.session_ttl_days", 30);
    SetKeyComment("emu.auth.session_ttl_days", "How long, in days, a login session stays valid while idle before the user must sign in again. Each use of a session refreshes this window. Set to 0 to never expire sessions.");

    SetKeyValueIfNotSet<int64_t>("emu.ranks.owner_user_id", 0);
    SetKeyComment("emu.ranks.owner_user_id", "The user id treated as rank 255 no matter what their User.Rank column says. This is the way back in if a rank edit locks you out of your own control panel, and the way to grant yourself the first rank on a fresh server. Set to 0 to disable.");

    SetKeyValueIfNotSet<int64_t>("emu.ranks.default_rank", kUserRankMember);
    SetKeyComment("emu.ranks.default_rank", "The rank number given to accounts created through the website's registration form. Accounts with no rank set at all are also treated as this rank.");

    SetKeyValueIfNotSet("emu.motd", "<h1>Welcome</h1><p>Welcome to my " NOOBWARRIOR_BRAND " server.</p><h2>Rules</h2><p>The operator of this server has not set any rules. However, don't take this as an opportunity to be a jackass and instead have some common courtesy.</p>");

    SetKeyValueIfNotSet("emu.autostart", true);
    SetKeyComment("emu.autostart", "If false, the server emulator does not start on launch.");

    SetKeyValueIfNotSet<uint16_t>("emu.http_port", 8080);
    SetKeyComment("emu.http_port", "The port that the HTTP server emulator should listen on.");

    SetKeyValueIfNotSet<uint16_t>("emu.https_port", 53640);
    SetKeyComment("emu.https_port", "The port that the HTTPS server emulator should listen on.");

    SetKeyValueIfNotSet("emu.public_ip", "");
    SetKeyComment("emu.public_ip", "The public address that remote clients should use to reach your game servers (e.g. 108.17.63.2 or a domain name). Leave empty to auto-detect your WAN IP via an external service. Set this manually if you are behind NAT/port-forwarding, use a static address, or auto-detection picks the wrong interface.");

    SetKeyValueIfNotSet<int>("emu.voice_turn.port", 3478);
    SetKeyComment("emu.voice_turn.port", "The public and local UDP port used by the built-in voice TURN relay. Forward this UDP port from your router to the host.");

    SetKeyValueIfNotSet<int>("emu.voice_turn.relay_port_begin", 49160);
    SetKeyValueIfNotSet<int>("emu.voice_turn.relay_port_end", 49200);
    SetKeyComment("emu.voice_turn.relay_port_begin", "The first UDP media relay port used by voice TURN allocations. Forward the complete configured range to the host.");
    SetKeyComment("emu.voice_turn.relay_port_end", "The last UDP media relay port used by voice TURN allocations. Forward the complete configured range to the host.");

    SetKeyValueIfNotSet<int>("emu.voice_turn.credential_ttl_seconds", 3600);
    SetKeyComment("emu.voice_turn.credential_ttl_seconds", "How long LocalRCC-issued TURN credentials remain valid. Active Players request replacements before expiry.");

    SetKeyValueIfNotSet<int>("emu.voice_turn.max_allocations", 64);
    SetKeyComment("emu.voice_turn.max_allocations", "Maximum simultaneous allocations accepted by the built-in voice TURN relay. Each Player can require both publishing and subscription allocations.");

    SetKeyValueIfNotSet("emu.asset_grab_mode", false);
    SetKeyComment("emu.asset_grab_mode", "If enabled, any asset that is retrieved from Roblox services will be downloaded and saved to a database of your choice. This requires emu.enable_roblox_proxy to be enabled.");

    SetKeyValueIfNotSet("emu.asset_grab_db", "");
    SetKeyComment("emu.asset_grab_db", "Where should the Asset Grab Mode save its assets to?");

    SetKeyValueIfNotSet("emu.video.transcode", true);
    SetKeyComment("emu.video.transcode", "Convert uploaded videos into a format the Roblox engine can play. Requires ffmpeg; if it is missing, videos are stored exactly as uploaded and will only play if they already happen to be readable.");

    SetKeyValueIfNotSet("emu.video.ffmpeg_path", "");
    SetKeyComment("emu.video.ffmpeg_path", "Path to the ffmpeg executable, or to the directory holding ffmpeg and ffprobe. Leave empty to look next to the program first and then on PATH.");

    SetKeyValueIfNotSet("emu.video.format", "auto");
    SetKeyComment("emu.video.format", "How videos are delivered to the engine. The stored asset is never modified; this only chooses what is served from it. \"auto\", the default, serves HLS: the first request for a video queues background segmenting and is answered with the stored file, and later requests get the segments once they are ready. \"mp4\" always serves the stored file, which needs no segment requests but cannot be seeked before it has downloaded.");

    SetKeyValueIfNotSet<int64_t>("emu.video.max_height", 720);
    SetKeyComment("emu.video.max_height", "Videos taller than this are scaled down. Set to 0 to keep the original resolution.");

    SetKeyValueIfNotSet<int64_t>("emu.video.bitrate_kbps", 2000);
    SetKeyComment("emu.video.bitrate_kbps", "Target video bitrate in kbit/s when re-encoding.");

    SetKeyValueIfNotSet<int64_t>("emu.video.audio_kbps", 128);
    SetKeyComment("emu.video.audio_kbps", "Target audio bitrate in kbit/s when re-encoding.");

    SetKeyValueIfNotSet<int64_t>("emu.video.segment_seconds", 6);
    SetKeyComment("emu.video.segment_seconds", "Target length of each HLS segment, in seconds. Shorter segments start playing sooner and seek more precisely, at the cost of more requests.");

    SetKeyValueIfNotSet("emu.video.allow_remux", true);
    SetKeyComment("emu.video.allow_remux", "If the uploaded video is already H.264 and small enough, repackage it without re-encoding. Much faster and lossless, but relies on the file being correctly described by its own headers.");

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
    SetKeyComment("emu.roles", "The ranks that exist on this server, as a list of { name = \"...\", rank = 0-255 } entries. Ranks must be unique; higher means more privileged. Rank 0 is whoever has no account at all, including guests. Rank 255 can always do everything, so a misconfigured emu.permissions cannot lock you out.");

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

    SetKeyValueIfNotSet("emu.permissions.forums.post", 1);
    SetKeyComment("emu.permissions.forums.post", "What rank is able to create forum threads and reply to them?");

    SetKeyValueIfNotSet("emu.permissions.forums.moderate", 100);
    SetKeyComment("emu.permissions.forums.moderate", "What rank is able to edit or delete somebody else's forum post? Editing and deleting your own post only requires emu.permissions.forums.post.");

    SetKeyValueIfNotSet("emu.permissions.forums.structure", 200);
    SetKeyComment("emu.permissions.forums.structure", "What rank is able to create, rename and delete the forum categories and forums themselves?");

    SetKeyValueIfNotSet("emu.permissions.items.write", 200);
    SetKeyComment("emu.permissions.items.write", "What rank is able to add or edit items (assets, badges, universes, users and so on) in the mounted databases through the website's control panel?");

    SetKeyValueIfNotSet("emu.permissions.federation.manage", 200);
    SetKeyComment("emu.permissions.federation.manage", "What rank is able to add, ban or auto-accept federation peers on the master server? This decides which other servers yours will trust identities from, so it should stay high.");

    SetKeyValueIfNotSet("emu.permissions.workshop.moderate", 100);
    SetKeyComment("emu.permissions.workshop.moderate", "What rank is able to edit or delete somebody else's master server workshop upload?");

    SetKeyValueIfNotSet("emu.permissions.control_panel.execute", 255);
    SetKeyComment("emu.permissions.control_panel.execute", "What rank is able to use the control panel sections that run code or touch files on the machine hosting the server - the Lua shell, the file system browser and the emergency controls? This is host access rather than game administration, so it defaults to the highest rank.");

    SetKeyValueIfNotSet("emu.permissions.control_panel.ranks", 255);
    SetKeyComment("emu.permissions.control_panel.ranks", "What rank is able to edit the rank list in emu.roles and the permissions in emu.permissions? Anybody with this can grant themselves anything, so it defaults to the highest rank.");

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
