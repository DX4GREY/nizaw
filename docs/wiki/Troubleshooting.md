# Troubleshooting

## Build fails because `pkg-config` or system headers are missing

Install the required development packages for your distribution, then rebuild:

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
```

## `libsystemd` is not found

This is usually not fatal. The project will build with a stub service implementation if systemd support is unavailable.

## The CLI says an option is unknown

Use `--help` to confirm the supported flags and subcommands:

```bash
./build/nizaw --help
```

## Plugin discovery returns nothing

Make sure the plugin directory exists and contains a valid `.so` file with the expected descriptor contract. You can point the command at a known folder explicitly:

```bash
./build/nizaw plugins list ./plugins
```

## Tests fail

Re-run the test suite and inspect the failing output:

```bash
ctest --test-dir build --output-on-failure -V
```

## Common environment assumptions

- Nizaw expects a Linux environment and direct access to Linux kernel interfaces
- Some commands may report limited or empty data when the process lacks permission or the device is unavailable
- The project is intended to be used as a library and CLI, not as a replacement for full service management software
