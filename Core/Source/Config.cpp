// === noobWarrior ===
// File: Config.cpp
// Started by: Hattozo
// Started on: 3/8/2025
// Description: Tweaks various parameters of noobWarrior functionality
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Database/DatabaseManager.h>

#include <fstream>
#include <unistd.h>

using namespace NoobWarrior;

Config::Config(const std::filesystem::path &filePath, lua_State *luaState) : BaseConfig("config", filePath, luaState),
    Version(NOOBWARRIOR_CONFIG_VERSION),
    Api_AssetDownload("https://assetdelivery.roblox.com/v1/asset/?id={}"),
    Api_AssetDetails("https://economy.roblox.com/v2/assets/{}/details"),
    Binaries_WineExe("wine"),
    Gui_Theme(Theme::Default)
{}

ConfigResponse Config::Open() {
    if (const ConfigResponse res = BaseConfig::Open(); res != ConfigResponse::Success) return res;
    SetKeyValueIfNotSet("language", "en_US");
    SetKeyValueIfNotSet("gui.theme", "default");

    SetKeyValueIfNotSet("databases", table);
    SetKeyValueIfNotSet("plugins", table);

    SetKeyValueIfNotSet("api.asset_download", "https://assetdelivery.roblox.com/v1/asset/?id={}");
    SetKeyValueIfNotSet("api.asset_details", "https://economy.roblox.com/v2/assets/{}/details");

    SetKeyValueIfNotSet("httpserver.port", 53640);
    SetKeyComment("httpserver.port", "The port that the HTTP server should listen on.");

    SetKeyValueIfNotSet("httpserver.proxies", table);
    SetKeyComment("httpserver.proxies", "Any proxies in this list will be used as a fallback reverse proxy for API requests in case yours fail.");

    SetKeyValueIfNotSet("httpserver.enable_roblox_proxy", true);
    SetKeyComment("httpserver.enable_roblox_proxy", "Use the Roblox API as a fallback reverse proxy for API requests. Note that this requires the program to be logged in to your Roblox account in order for it to work.");

    SetKeyValueIfNotSet("httpserver.game_view_mode", "server");
    SetKeyComment("httpserver.game_view_mode", "This value can be set to three modes, \"server\", \"universe\", and \"place\". If set to \"server\", it will display individual servers on the website. If set to \"universe\", it will display an entire Roblox game containing all the servers being hosted for it and the places it contains; this is how Roblox does it for their website. If set to \"place\", individual places that belong to a universe will be shown; this is how Roblox did it before introducing the \"Universe\" system in 2014.");

    SetKeyValueIfNotSet("httpserver.connect_to_individual_place", true);

    SetKeyValueIfNotSet("httpserver.banner.background_color", "#ff8000");
    SetKeyValueIfNotSet("httpserver.banner.foreground_color", "#ffffff");
    SetKeyValueIfNotSet("httpserver.banner.message", "");
    SetKeyComment("httpserver.banner", "Customizes the banner that any visitor who connects to your website will see.");

    SetKeyValueIfNotSet("httpserver.logged_out_text", R"(
    You have arrived at the web interface for your noobWarrior server! For security reasons, authentication is required to do anything by default since it is likely that guests may stumble across this page.

    If you're using the GUI interface, you need to use the Database Editor and edit the master database to make your own user and give it admin privileges. After that, use the Settings menu to change this welcome text for logged out users so that they won't make fun of you for not changing it.

    If you are hosting this on a dedicated server, it is likely that you are running it on a Linux server without a GUI. Stop the service responsible for running the program, use "sudo -u (the user you are using to run the program) noobwarrior-cli". After entering the interactive shell, use "db --file master --add user --id 0 --name Admin --password yourpasswordhere --admin" to make a new account. Restart the service and login with that account and you should be good to go.
    )");
    SetKeyValueIfNotSet("httpserver.branding.title", "noobWarrior");
    SetKeyValueIfNotSet("httpserver.branding.logo", "/img/icon1024.png");
    SetKeyValueIfNotSet("httpserver.branding.favicon", "/img/favicon.ico");
    SetKeyValueIfNotSet("httpserver.branding.description", "My noobWarrior server");
    SetKeyComment("httpserver.branding", "The branding that people will see when they connect to your website.");

    SetKeyValueIfNotSet("httpserver.permissions.allow_registration", false);
    SetKeyComment("httpserver.permissions.allow_registration", "If this is set to true, registrations for guests will be disabled and administrators must manually create accounts in a database.");

    SetKeyValueIfNotSet("httpserver.permissions.enable_login_filter", false);
    SetKeyComment("httpserver.permissions.enable_login_filter", "Makes it so that any users who are blacklisted or not whitelisted will not be able to log in.");

    SetKeyValueIfNotSet("httpserver.permissions.login_filter_type", "whitelist");
    SetKeyComment("httpserver.permissions.login_filter_type", "This setting only applies if enable_login_filter is set to true. You can set this to either \"blacklist\" or \"whitelist\"");

    SetKeyValueIfNotSet("httpserver.permissions.ugc", "user");
    SetKeyComment("httpserver.permissions.ugc", "What rank is able to upload user generated content to the website?");

    SetKeyValueIfNotSet("httpserver.permissions.access.home", "user");
    SetKeyComment("httpserver.permissions.access.servers", "What rank is able to access the Home page on the website?");

    SetKeyValueIfNotSet("httpserver.permissions.access.servers", "user");
    SetKeyComment("httpserver.permissions.access.servers", "What rank is able to access the Servers page on the website?");

    SetKeyValueIfNotSet("httpserver.permissions.access.library", "user");

    SetKeyValueIfNotSet("httpserver.permissions.access.control_panel", "admin");
    SetKeyValueIfNotSet("httpserver.permissions.remote_start_server", "user");
    SetKeyComment("httpserver.permissions.remote_start_server", "What rank is able to remotely start game servers? Remotely starting game servers means being able to click \"Play\" on a game and having a server automatically launch for you if none exists.");

    SetKeyValueIfNotSet("httpserver.permissions.connect_to_server", "guest");
    SetKeyComment("httpserver.permissions.connect_to_server", "What rank is able to connect to your server? By default, this is set to \"guest\" for convenience reasons.");

    SetKeyValueIfNotSet("httpserver.currency.enabled", false);
    SetKeyComment("httpserver.currency.enabled", "If this is enabled, users will have a visible Robux balance on the website and will be able to spend it.");

    SetKeyValueIfNotSet("httpserver.currency.enable_tix", true);
    SetKeyComment("httpserver.currency.enable_tix", "Enables the ability for people to spend Tickets, a freemium currency on Roblox that was removed in 2016.");

    SetKeyValueIfNotSet("httpserver.currency.make_everything_free", true);
    SetKeyComment("httpserver.currency.make_everything_free", "If this is enabled, everything on the website will be free regardless of the price it is set to. Note that disabling this and keeping the currency feature disabled will make no one be able to buy anything.");

    SetKeyValueIfNotSet("httpserver.currency.starting_robux", 0);
    SetKeyComment("httpserver.currency.starting_robux", "If this is set to a number above 0, any new registrars will receive the specified amount of Robux. This does not apply retroactively.");

    SetKeyValueIfNotSet("httpserver.currency.starting_tix", 0);
    SetKeyComment("httpserver.currency.starting_tix", "If this is set to a number above 0, any new registrars will receive the specified amount of Tix. This does not apply retroactively.");

    SetKeyValueIfNotSet("httpserver.currency.daily_robux", 0);
    SetKeyValueIfNotSet("httpserver.currency.daily_tix", 0);

    SetKeyValueIfNotSet("wine.exe", "wine");
    SetKeyValueIfNotSet("wine.prefix", "");
    return ConfigResponse::Success;
}
