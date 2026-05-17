# Changelog

## Version Custom

- Information:
  - This a modification of AntonioND's ArchitectDS build system made in python for simpler
  bash assets automation. This custom version feature support of my nitro-engine-advanced fork.

## Version 0.5.0

- Core:

  - Use the default animated ROM icon of BlocksDS instead of the static one.
  - Allow building individual source code files, not just full folders.
  - Improve parsing of `.ptxc` config files for ptexconv.
  - Support `.grf` files as output of ptexconv.
  - Update the URL to the BlocksDS website in the default ROM header strings.
  - Update the URL to ArchitectDS in the wheel configuration and the
    documentation.
  - Remove the C and C++ standards defined by default in ArchitectDS. Now the
    build will be done using the compiler default settings (or the one specified
    by the user if any).
  - Pass all environment variables to ninja, not just `BLOCKSDS`.

- Examples:

  - Improve ROM header information of `all` libnds example.
  - Import some examples from BlocksDS for reference.
  - Update the code of the ARM7 binaries in the examples.
  - Update an example to Show how to select individual source code files to be
    built.

## Version 0.4.3

- Core:

  - Support generating one `.mas` file per audio file with mmutil.
  - Let user specify DSi title ID and unit code. @jonko0493

## Version 0.4.2

- Core:

  - Remove requirement for mmutil to not run in parallel now that the
    bug has been fixed in mmutil.

## Version 0.4.1

- Core:

  - Support passing arguments to ptexconv with a `.ptxc` file in a similar way as
    grit with `.grit` files. @jonko0493

## Version 0.4.0

- Core:

  - Support generating `compile_commands.json` files.
  - Update setup instructions and document the generation of
    `compile_commands.json` files.
  - Update dynamic library linker options.
  - Format the game tile correctly if the game subtitle isn't provided. @Cavv

## Version 0.3.1

- Core:

  - Fix linker invocation in C++ projects.
  - Let user specify any pre-built ARM7 core, not just the default one.

- Examples:

  - Add example of using LibXM7 from files in NitroFS.

## Version 0.3.0

- Core:

  - Add DSF files to dependency list correctly.
  - Simplify invocation of squeezerw.
  - Support GL2D sprite sets injected to ARM9 binaries and saved to filesystems.
  - Allow users to specify `LDFLAGS`.
  - Add initial support for ARM9 dynamic libraries. It's possible to create
    dynamic libraries that require the main binary (similar to overlays) or
    standalone (such as user plugins).
  - Some minor internal changes.

- Examples:

  - Add example of using GL2D sprite sets from the ARM9 or the filesystem.
  - Check for errors loading 3D textures in the examples.
  - Fix `CFLAGS` in debug builds of the debugging example.
  - Add the full license text to the repository, not just its name.

## Version 0.2.1

- Core:

  - Suppress warning from system includes.

- Examples:

  - Add examples of using libdsf.
  - Add examples of using rich text with Nitro Engine.
  - Add example of using volumetric shadows with Nitro Engine.

## Version 0.2.0

- Core:

  - Support `.fnt` BMFont files.

- Examples:

  - Improve conversion arguments of 16-bit textures.

## Version 0.1.2

- Core:

  - Fix invocation of python scripts from ninja rules.

## Version 0.1.1

- Core:

  - Fix environment variables on Windows.

- Examples:

  - Add gbajpeg example.

## Version 0.1.0

- Core:

  - Initial version.
  - It supports ARM9 and ARM7 binaries, NitroFS and FAT filesystems, and an
    arbitrary number of DSP binaries (that can be in the ARM9 binary or the
    filesystem).

- Examples:

  - Several examples have been created to show how to use the build system. They
    show how to use it with libnds, Nitro Engine, NFlib, Maxmod, etc.
