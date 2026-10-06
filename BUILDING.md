# Building
# Dependencies
These dependencies are required to be pre-installed on your system:
- Git
- CMake
- Qt 6 (only if you're building the GUI executable, which in most cases you will be.)

## Windows
### Installing
Install [Git for Windows](https://git-scm.com/install/windows) by going to the website, and going through the setup wizard. Add them to PATH if they ask you to.

I've seen more stability with Git for Windows than the one shipped through MSYS2. It treats line endings using CRLF.

Then you'll need a compiler toolchain so that your machine is able to assemble the source code into an .exe file.

The recommended way of acquiring a compiler is using [MSYS2](https://www.msys2.org), which is a program that provides a Unix-like environment on Windows so that you can use package managers. It comes with Pacman, which is a package manager that you may recognize if you've ever used Arch Linux before.

We personally use the clang64 toolchain on [MSYS2](https://www.msys2.org), you can install it by typing into the shell: ``pacman -S mingw-w64-clang-x86_64-toolchain``

Also install CMake: ``pacman -S mingw-w64-clang-x86_64-cmake``

And install a build system, like Ninja: ``pacman -S mingw-w64-clang-x86_64-ninja``

Lastly, install Qt 6: ``pacman -S mingw-w64-clang-x86_64-qt6-base mingw-w64-clang-x86_64-qt6-multimedia``

### Adding MSYS2 to PATH
You need a way for your computer to find where all these programs are. That's where the PATH comes into play.

Go to Control Panel, click "System and Security", then click "System", and then click "Advanced system settings" on the left sidebar. It should open up a dialog named "System Properties." This is where you'll find a button named "Environment Variables..." on the bottom right. Click that.

After you've clicked it, it should open a window named Environmnent Variables. Here you can configure the user and system variables. You'll want to configure the system-wide ones, so look at the bottom panel of the dialog. There should be a variable named "Path", containing all the locations that the system will try to find binaries in. Double-click it, and it should open another window.

In this window, click "New" on the top right and add the location "C:\msys64\clang64\bin", and also "C:\msys64\usr\bin" if you want to have access to coreutils in your terminal.

Click OK on all dialog boxes, restart your terminal, and you should be done!

### Cloning the repository
Change directory to whatever folder you want the repository in. I recommend your documents folder, so do ``cd Documents``

Then run ``git clone https://github.com/noobwarrior-org/noobwarrior.git``

Cd into it: ``cd noobwarrior``

### Compiling everything through the terminal (except for noobHook and the DLL injector)
Create a "build" folder using ``mkdir build``

Let CMake initialize a folder for itself using ``cmake . -B ./build -G "Ninja"``. If you don't have Ninja, change it to a generator of your choice.

Change directory to the build folder using ``cd build``

Then check what build system it's using by doing ``dir`` and checking if there's a file named ``Makefile`` or ``build.ninja``

Then run either ``ninja`` or ``make``

### Setting up Visual Studio Code and compiling everything through there (except for noobHook and the DLL injector)
If you prefer using an IDE instead of the terminal, I recommend Visual Studio Code. (I use VSCodium personally.)

Install these extensions:
- CMake Tools
- clangd

### Compiling noobHook + the DLL Injector
MSVC does not work for compiling the core parts of the project.

However, noobHook and the DLL injector do, in fact, require MSVC in order to be compiled correctly. They will not compile otherwise.

## Mac
Coming soon!

## Linux
### Arch-derivative distributions
If you've been using Arch or any other Arch-derivative distribution for more than 5 seconds, you should usually already have a compiler installed.

If not, you can install GCC by typing in the terminal: ``sudo pacman -S gcc``

More is coming soon.