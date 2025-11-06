// === noobWarrior ===
// File: Config.cpp
// Started by: Hattozo
// Started on: 3/8/2025
// Description: Tweaks various parameters of noobWarrior functionality
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Database/DatabaseManager.h>

#include <fstream>
#include <nlohmann/json_fwd.hpp>
#include <unistd.h>

using namespace NoobWarrior;

Config::Config(const std::filesystem::path &filePath, lua_State *luaState) : BaseConfig("config", filePath, luaState)
{}

ConfigResponse Config::Open() {
    if (const ConfigResponse res = BaseConfig::Open(); res != ConfigResponse::Success) return res;
    SetKeyValue("meta.version", NOOBWARRIOR_CONFIG_VERSION);
    SetKeyValueIfNotSet("language", "en_US");
    SetKeyValueIfNotSet("gui.theme", "default");

    nlohmann::json defaultMasterServer = nlohmann::json::object();
    defaultMasterServer["url"] = "https://community.noobwarrior.org";

    SetKeyValueIfNotSet<nlohmann::json>("gui.master_servers", { defaultMasterServer });

    SetKeyValueIfNotSet<nlohmann::json>("databases", {});
    SetKeyValueIfNotSet<nlohmann::json>("plugins", {});

    SetKeyValueIfNotSet("internet.index", "https://raw.githubusercontent.com/noobwarrior-org/index/refs/heads/main/index.json");
    SetKeyValueIfNotSet("internet.roblox.asset_download", "https://assetdelivery.roblox.com/v1/asset/?id={}");
    SetKeyValueIfNotSet("internet.roblox.asset_details", "https://economy.roblox.com/v2/assets/{}/details");

    SetKeyValueIfNotSet("httpserver.branding.title", "noobWarrior");
    SetKeyValueIfNotSet("httpserver.branding.logo", "/img/icon1024.png");
    SetKeyValueIfNotSet("httpserver.branding.favicon", "/img/favicon.ico");
    SetKeyValueIfNotSet("httpserver.branding.tagline", "My noobWarrior server");
    SetKeyComment("httpserver.branding", "The branding that people will see when they connect to your website.");

    SetKeyValueIfNotSet("httpserver.auth.type", "master");
    SetKeyComment("httpserver.auth.type", "If set to \"master\", your server is responsible for all authentication. If set to \"slave\", the server URL set in the \"master\" variable will be responsible for all authentication.");

    SetKeyValueIfNotSet("httpserver.auth.master", "https://servers.noobwarrior.org");
    SetKeyComment("httpserver.auth.master", "The URL of the server that your server's authentication system accepts. Does nothing if the auth type is set to \"master\"");

    SetKeyValueIfNotSet("httpserver.auth.allow_registration", false);
    SetKeyComment("httpserver.auth.allow_registration", "If this is set to false, registrations for guests will be disabled and administrators must manually create accounts in a database.");

    SetKeyValueIfNotSet("httpserver.auth.password_based", true);
    SetKeyComment("httpserver.auth.password_based", "If true, enables password-based authentication. If set to false, then the only way the user will be allowed to login is if they use OAuth2 based services.");

    SetKeyValueIfNotSet("httpserver.auth.enable_login_filter", false);
    SetKeyComment("httpserver.auth.enable_login_filter", "Makes it so that any users who are blacklisted or not whitelisted will not be able to log in.");

    SetKeyValueIfNotSet("httpserver.auth.login_filter_type", "whitelist");
    SetKeyComment("httpserver.auth.login_filter_type", "This setting only applies if enable_login_filter is set to true. You can set this to either \"blacklist\" or \"whitelist\"");

    SetKeyValueIfNotSet("httpserver.auth.enable_custom_pfp", false);
    SetKeyComment("httpserver.auth.enable_custom_pfp", "Allows users to upload their own profile pictures instead of using their avatar headshot.");

    SetKeyValueIfNotSet("httpserver.emulator.motd", "<h1>Welcome</h1><br><p>Welcome to my noobWarrior server.</p><br><h2>Rules</h2><br><p>The operator of this server has not set any rules. However, don't take this as an opportunity to be a jackass and instead have some common courtesy.</p>");

    SetKeyValueIfNotSet("httpserver.emulator.port", 53640);
    SetKeyComment("httpserver.emulator.port", "The port that the server emulator should listen on.");

    SetKeyValueIfNotSet<nlohmann::json>("httpserver.emulator.proxies", {});
    SetKeyComment("httpserver.emulator.proxies", "Any proxies in this list will be used as a fallback reverse proxy for API requests in case yours fail.");

    SetKeyValueIfNotSet("httpserver.emulator.enable_roblox_proxy", true);
    SetKeyComment("httpserver.emulator.enable_roblox_proxy", "Use the Roblox API as a fallback reverse proxy for API requests. Note that this requires the program to be logged in to your Roblox account in order for it to work.");

    SetKeyValueIfNotSet("httpserver.emulator.game_view_mode", "server");
    SetKeyComment("httpserver.emulator.game_view_mode", "This value can be set to three modes, \"server\", \"game\", and \"place\". If set to \"server\", it will display individual servers on the website. If set to \"game\", it will display an entire Roblox game containing all the servers being hosted for it and the places it contains; this is how Roblox does it for their website. If set to \"place\", individual places that belong to a universe will be shown; this is how Roblox did it before introducing the \"Universe\" system in 2014.");

    SetKeyValueIfNotSet("httpserver.emulator.connect_to_individual_place", true);

    SetKeyValueIfNotSet("httpserver.emulator.banner.background_color", "#ff8000");
    SetKeyValueIfNotSet("httpserver.emulator.banner.foreground_color", "#ffffff");
    SetKeyValueIfNotSet("httpserver.emulator.banner.message", "");
    SetKeyComment("httpserver.emulator.banner", "Customizes the banner that any visitor who connects to your website will see.");

    SetKeyValueIfNotSet("httpserver.emulator.logged_out_text", R"(
    You have arrived at the web interface for your noobWarrior server! For security reasons, authentication is required to do anything by default since it is likely that guests may stumble across this page.

    If you're using the GUI interface, you need to use the Database Editor and edit the master database to make your own user and give it admin privileges. After that, use the Settings menu to change this welcome text for logged out users so that they won't make fun of you for not changing it.

    If you are hosting this on a dedicated server, it is likely that you are running it on a Linux server without a GUI. Stop the service responsible for running the program, use "sudo -u (the user you are using to run the program) noobwarrior-cli". After entering the interactive shell, use "db --file master --add user --id 0 --name Admin --password yourpasswordhere --admin" to make a new account. Restart the service and login with that account and you should be good to go.
    )");

    SetKeyValueIfNotSet("httpserver.emulator.permissions.upload_ugc", "user");
    SetKeyComment("httpserver.emulator.permissions.upload_ugc", "What rank is able to upload user generated content to the website?");

    SetKeyValueIfNotSet("httpserver.emulator.permissions.access.home", "user");
    SetKeyComment("httpserver.emulator.permissions.access.servers", "What rank is able to access the Home page on the website?");

    SetKeyValueIfNotSet("httpserver.emulator.permissions.access.servers", "user");
    SetKeyComment("httpserver.emulator.permissions.access.servers", "What rank is able to access the Servers page on the website?");

    SetKeyValueIfNotSet("httpserver.emulator.permissions.access.library", "user");

    SetKeyValueIfNotSet("httpserver.emulator.permissions.access.control_panel", "admin");
    SetKeyValueIfNotSet("httpserver.emulator.permissions.remote_start_server", "user");
    SetKeyComment("httpserver.emulator.permissions.remote_start_server", "What rank is able to remotely start game servers? Remotely starting game servers means being able to click \"Play\" on a game and having a server automatically launch for you if none exists.");

    SetKeyValueIfNotSet("httpserver.emulator.permissions.connect_to_server", "guest");
    SetKeyComment("httpserver.emulator.permissions.connect_to_server", "What rank is able to connect to your server? By default, this is set to \"guest\" for convenience reasons.");

    SetKeyValueIfNotSet("httpserver.emulator.currency.enabled", false);
    SetKeyComment("httpserver.emulator.currency.enabled", "If this is enabled, users will have a visible Robux balance on the website and will be able to spend it.");

    SetKeyValueIfNotSet("httpserver.emulator.currency.enable_tix", true);
    SetKeyComment("httpserver.emulator.currency.enable_tix", "Enables the ability for people to spend Tickets, a freemium currency on Roblox that was removed in 2016.");

    SetKeyValueIfNotSet("httpserver.emulator.currency.make_everything_free", true);
    SetKeyComment("httpserver.emulator.currency.make_everything_free", "If this is enabled, everything on the website will be free regardless of the price it is set to. Note that disabling this and keeping the currency feature disabled will make no one be able to buy anything.");

    SetKeyValueIfNotSet("httpserver.emulator.currency.starting_robux", 0);
    SetKeyComment("httpserver.emulator.currency.starting_robux", "If this is set to a number above 0, any new registrars will receive the specified amount of Robux. This does not apply retroactively.");

    SetKeyValueIfNotSet("httpserver.emulator.currency.starting_tix", 0);
    SetKeyComment("httpserver.emulator.currency.starting_tix", "If this is set to a number above 0, any new registrars will receive the specified amount of Tix. This does not apply retroactively.");

    SetKeyValueIfNotSet("httpserver.emulator.currency.daily_robux", 0);
    SetKeyValueIfNotSet("httpserver.emulator.currency.daily_tix", 0);

    SetKeyComment("httpserver.master", "Options to configure the master server. The master server handles the server listings and authentication for all server emulators attached to it.");
    
    SetKeyValueIfNotSet("httpserver.master.port", 9999);
    SetKeyComment("httpserver.master.port", "The port that the master server should listen on.");

    SetKeyValueIfNotSet("httpserver.master.motd", "<h1>Welcome</h1><br><p>Hi, welcome to my server list.</p><br><h2>Rules</h2><br><p>The operator of this server list has not set any rules. However, don't take this as an opportunity to be a jackass and instead have some common courtesy.</p>");

    SetKeyValueIfNotSet("wine.exe", "wine");
    SetKeyValueIfNotSet("wine.prefix", "");
    return ConfigResponse::Success;
}
