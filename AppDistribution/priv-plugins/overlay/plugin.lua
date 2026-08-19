return {
    identifier = "overlay@noobwarrior.org",
    title = "Overlay",
    version = "v1.0",
    description = "Provides an in-game overlay and console for noobWarrior.",
    authors = { "hattozo@noobwarrior.org" },
    datamodel = {
        {
            dir = "datamodel",
            side = "Server",
            predicate = function(data)
                return true
            end
        }
    },
}