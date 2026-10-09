return {
    {
        Title = "docs.getting_started",
        Fallback = "Getting Started",
        Children = {
            {
                Title = "docs.getting_started.preface",
                Fallback = "Preface",
                File = "001-getting-started/001-preface.md"
            },
            {
                Title = "docs.getting_started.introduction",
                Fallback = "Introduction",
                File = "001-getting-started/002-introduction.md"
            }
        }
    },
    {
        Title = "docs.user_guides",
        Fallback = "User Guides"
    },
    {
        Title = "docs.api_reference",
        Fallback = "API Reference",
        Children = {
            {
                Title = "docs.api_reference.lua",
                Fallback = "Lua"
            },
            {
                Title = "docs.api_reference.cpp",
                Fallback = "C++"
            },
            {
                Title = "docs.api_reference.c",
                Fallback = "C"
            }
        }
    }
}