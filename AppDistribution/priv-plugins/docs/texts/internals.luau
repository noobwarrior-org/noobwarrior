local function page(key, fallback, file)
    return { Title = "docs.internals." .. key, Fallback = fallback, File = file }
end

return {
    page("overview", "Overview", "overview.md"),
    page("building", "Building and testing", "building.md"),
    {
        Title = "docs.internals.subsystems",
        Fallback = "Subsystems",
        Children = {
            page("engines", "Engines", "engines.md"),
            page("server_emulator", "Server emulator", "server-emulator.md"),
            page("authentication", "Authentication", "authentication.md"),
            page("emudb", "EmuDb", "emudb.md"),
            page("plugins", "Plugins", "plugins.md"),
            page("lhp", "LHP", "lhp.md"),
            page("files_and_urls", "Files and URLs", "files-and-urls.md"),
            page("registry", "Registry", "registry.md"),
            page("roblox_files", "Roblox files", "roblox-files.md")
        }
    }
}
