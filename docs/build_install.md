# Building and Installing Hera

This documentation page provides the necessary instructions to build Hera from source and
install the resulting binaries and plugins on a target system.

## Build Requirements
The following dependencies are required to build Hera:

### Mandatory dependencies
- **C11 Compiler**: A standard-compliant C compiler with GNU extensions, such
    as `gcc` or `clang`
- **GNU Make**: The build system utilizes GNU Make extensions and thus requires
    a compatible version

### Optional dependencies
- **Tree-sitter CLI**: Required to generate the Tree-sitter parser from the
    provided grammar
- **vsce**: Required to package the Visual Studio Code extension
- **Clang & LLVM**: Required for building and executing the fuzzer

## Build targets
The Hera `GNUmakefile` defines several targets to manage the build process:

### Tool
The `tool` target builds the core `hera` command-line utility.
```bash
make tool
```
The resulting binary will be placed in `bin/hera`.

### Extra
The `extra` target builds the supporting plugins and extensions.
```bash
make extra
```
- **Tree-sitter Parser**: Generates and compiles the Tree-sitter parser into
    `bin/treesitter-hera.so`
- **VS Code Extension**: Packages the extension into `bin/hera.vsix`

### Fuzzer
The `fuzzer` target builds the libFuzzer-based diagnostic tool.
```bash
make fuzzer
```
> [!IMPORTANT]
> This target explicitly requires `clang` as the compiler and `llvm-symbolizer`
> for effective report generation.

### All
The default target. It builds both the `tool` and `extra` targets.
```bash
make all
```

## Installation
Hera supports installation to the local file system using the `install` target.

### Default Installation
By default, the `hera` binary is installed to `/usr/bin`.
```bash
sudo make install
```

### Custom prefix
The installation directory may be customized by overriding the `PREFIX` variable.
```bash
make install PREFIX=$HOME/.local/bin
```

### Uninstallation
The `uninstall` target removes the `hera` binary from the system. Ensure
the `PREFIX` matches the one used during installation.
```bash
sudo make uninstall
```

## Quality assurance

### Fuzzing
Hera includes a fuzzing suite to verify the stability of the tool against
malformed input.
```bash
make fuzz
```
This command builds the fuzzer, prepares a corpus from the `examples/`
directory, and begins the fuzzing process with AddressSanitizer (ASan) and
UndefinedBehaviorSanitizer (UBSan) enabled.

### Cleaning
To remove all build artifacts:
```bash
make clean
```
To also remove the fuzzer corpus and outputs:
```bash
make purge
```
