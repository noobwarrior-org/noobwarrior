return {
    {
        Title = "docs.api_reference.luau",
        Fallback = "Luau",
        Children = {
            {
                Title = "docs.api_reference.luau.globals",
                Fallback = "Globals",
                Children = {
                    {
                        Title = "loadstring",
                        File = "luau/globals/loadstring.md"
                    }
                }
            },
            {
                Title = "docs.api_reference.luau.libraries",
                Fallback = "Libraries",
                Children = {
                    {
                        Title = "reg",
                        Children = {
                            {
                                Title = "GetKeyValue",
                                File = "luau/libraries/reg/GetKeyValue.md"
                            }
                        },
                        File = "luau/libraries/reg/reg.md"
                    }
                }
            },
            {
                Title = "docs.api_reference.luau.enums",
                Fallback = "Enumerations",
                Children = {
                    {
                        Title = "ItemType",
                        File = "luau/enums/ItemType.md"
                    }
                }
            }
        }
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
