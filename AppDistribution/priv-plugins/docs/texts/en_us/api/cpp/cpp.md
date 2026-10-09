# C++
All of noobWarrior is in one library, `NoobWarrior.Core`. The desktop app, the command-line app and the Android app are thin front ends over it. You can link it into your own program to get the same server emulator, databases and plugin system.

# Linking
Add noobWarrior's source tree to your CMake project with `add_subdirectory` and link the `NoobWarrior.Core` target. Turn off the targets you don't need, such as `NOOBWARRIOR_TARGET_QTGUI`. The target adds its include folder, `Core/Include`, for you. The library builds as a static library by default; set `NOOBWARRIOR_LIBTYPE_STATIC` to `OFF` for a shared one.

```cpp
#include <NoobWarrior/NoobWarrior.h>
```

Everything is in the `NoobWarrior` namespace.

# Starting noobWarrior
Create one [Core](/api/cpp/classes/Core). Its constructor sets everything up: the registry, databases, plugins and the server emulator.

noobWarrior has no thread of its own. Your program must call `Core::ProcessEvents` over and over, or no web requests are answered. The desktop app calls it from a timer.

```cpp
#include <NoobWarrior/NoobWarrior.h>

int main(int argc, char **argv) {
    NoobWarrior::Core core({ .ArgCount = argc, .ArgVec = argv });
    if (core.Fail())
        return 1;

    while (true)
        core.ProcessEvents(true);
}
```

# Conventions
- Methods are `PascalCase` and member variables start with `m`.
- Operations that can fail return an `enum class Response` or `FailReason` scoped to their class. They don't throw.
- Objects that can fail while being built have a `Fail()` method. Check it right after constructing one.
