# Team Fortress Invasion

Team Fortress Invasion is a project that aims to replicate once cannceled iteration of valve's Team Fortress 2 based on Source SDK 2013.

e.g. This project is not associated with Valve and Team Fortress/Team Fortress Invasion trademarks belong to Valve and poject name will be changed in future.
 
## Build instructions

Clone the repository using the following command:
```bash
git clone https://github.com/quellerz/team-fortress-invasion.git
```
### Windows

Requirements:
 - Source SDK 2013 Multiplayer installed via Steam
 - Visual Studio 2022 with the following workload and components:
   - Desktop development with C++:
     - MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest)
     - Windows 11 SDK (10.0.22621.0) or Windows 10 SDK (10.0.19041.1)
 - Python 3.13 or later
 - DirectX SDK 2008
 - Strawberry Perl with following packages:
   - String::CRC32

Inside the cloned directory, navigate to `src`, run:
```bat
createallprojects.bat
```
This will generate the Visual Studio project `everything.sln` which will be used to build your mod.

Then, on the menu bar, go to `Build > Build Solution`, and wait for everything to build.

You can then select the `Client (Mod Name)` project you wish to run, right click and select `Set as Startup Project` and hit the big green `> Local Windows Debugger` button on the tool bar in order to launch your mod.

The default launch options should be already filled in for the `Release` configuration.

#### Shaders

This project utilizes **LUX** shaders. To build **LUX**:
1. Run `src\materialsystem\stdshaders\!Compile_All.bat` to compile all the Shaders.<br>
The Time this takes depends on your Hardware. This Step is CPU intensive.<br>
2. Run ```createallprojects.bat ``` to generate the `.sln` Solution File.
3. Compile the `game_shader_generic_example.dll` using the 'Shaders' Project in the .sln File.<br>
( This results in a `game_shader_dx9.dll` for SDK2013SP )<br>
4. Copy the compiled Shaders found in `game\mod_tf\shaders\fxc\` to your `mod\shaders\fxc\` Folder.<br>
And the previously mentioned .dll File from `game\mod_tf\bin\` your `mod\bin\` Folder.<br>

### Linux

Requirements:
 - Source SDK 2013 Multiplayer installed via Steam
 - podman

Inside the cloned directory, navigate to `src`, run:
```bash
./buildallprojects
```

This will build all the projects related to the SDK and your mods automatically against the Steam Runtime.

You can then, in the root of the cloned directory, you can navigate to `game` and run your mod by launching the build launcher for your mod project, eg:
```bash
./tf_invasion
```

*Mods that are distributed on Steam MUST be built against the Steam Runtime, which the above steps will automatically do for you.*

## Extra links

- [Team Fortress Invasion game design document](https://github.com/quellerz/team-fortress-invasion-design-document)

## License

The SDK is licensed to users on a non-commercial basis under the [SOURCE 1 SDK LICENSE](LICENSE), which is contained in the [LICENSE](LICENSE) file in the root of the repository.

For more information, see [Distributing your Mod](#markdown-header-distributing-your-mod).
