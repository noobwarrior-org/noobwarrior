local function pages(folder, names)
    local list = {}
    for _, name in names do
        table.insert(list, { Title = name, File = folder .. "/" .. name .. ".md" })
    end
    return list
end

local function library(name, functions)
    return {
        Title = name,
        File = "luau/libraries/" .. name .. "/" .. name .. ".md",
        Children = functions and pages("luau/libraries/" .. name, functions) or nil
    }
end

return {
    {
        Title = "docs.api_reference.luau",
        Fallback = "Luau",
        File = "luau/luau.md",
        Children = {
            {
                Title = "docs.api_reference.luau.globals",
                Fallback = "Globals",
                Children = pages("luau/globals", {
                    "emu", "emu_db_mgr", "loadstring", "plugin", "print", "require", "script"
                })
            },
            {
                Title = "docs.api_reference.luau.libraries",
                Fallback = "Libraries",
                Children = {
                    library("core", {
                        "BuildAvatarFetchJson", "ConsoleAdded", "GetEmuDbManager", "GetInstallDataDir",
                        "GetMasterDatabase", "GetPermissionRank", "GetPluginDataDir", "GetUserDataDir",
                        "GetVersion", "HasPermission", "ResolveSession"
                    }),
                    library("crypto", { "GenerateEd25519", "Sign", "Verify" }),
                    library("hash", { "GenerateToken", "HashPassword", "Sha256", "VerifyPassword" }),
                    library("json"),
                    library("lang", { "GetAvailableLanguages", "GetCode", "Has", "Translate" }),
                    library("lhp", { "Render", "RenderFile" }),
                    library("reg", { "GetKeyValue", "Save", "SetKeyComment", "SetKeyValue", "SetKeyValueIfNotSet" }),
                    library("serpent"),
                    library("url", {
                        "GetCwd", "GetDirectory", "GetFileName", "GetHostName", "GetProtocol",
                        "GetProtocolString", "GetVfs", "Resolve", "ResolveAsLocalPath", "ResolveAsPath", "ResolveAsVfsPath",
                        "ResolveFromCaller", "ResolveWithoutProtocol"
                    })
                }
            },
            {
                Title = "docs.api_reference.luau.classes",
                Fallback = "Classes",
                Children = pages("luau/classes", {
                    "CommandContext", "Console", "EmuDb", "EmuDbManager", "HttpRequest", "HttpServer",
                    "NetClient", "Plugin", "Script", "ServerEmulator", "Signal", "SignalListener", "SqlDb",
                    "VirtualFileSystem"
                })
            },
            {
                Title = "docs.api_reference.luau.enums",
                Fallback = "Enumerations",
                Children = pages("luau/enums", {
                    "AssetType", "CompressionType", "CreatorType", "ItemType", "Permission", "ProtocolType",
                    "SqlResponse"
                })
            }
        }
    },
    {
        Title = "docs.api_reference.cpp",
        Fallback = "C++",
        File = "cpp/cpp.md",
        Children = {
            {
                Title = "docs.api_reference.cpp.classes",
                Fallback = "Classes",
                Children = pages("cpp/classes", { "Core", "Registry" })
            },
            {
                Title = "docs.api_reference.cpp.enums",
                Fallback = "Enumerations",
                Children = pages("cpp/enums", { "ItemType" })
            }
        }
    },
    {
        Title = "docs.api_reference.c",
        Fallback = "C"
    }
}
