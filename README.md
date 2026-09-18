# malasim

## Build Instructions

### Prerequisites

1. Install [vcpkg](https://github.com/microsoft/vcpkg).
2. Install dependencies using vcpkg:
    ```sh
    ./vcpkg install gsl yaml-cpp fmt libpq libpqxx sqlite3 date args cli11 gtest catch easyloggingpp
    ```

### Building the Project

To build the project, run the following commands:

```sh
./scripts/build.sh
```

Alternatively, you can use the `Makefile` targets:

```sh
make install-deps
make generate
make build
make test
make run
```

### Running Tests

By default, test output is concise (warn level). To enable verbose logging:

```sh
MALASIM_LOG_LEVEL=info ./build/bin/malasim_test
```

Available levels: `trace`, `debug`, `info`, `warn`, `err`, `critical`

### Simulation Output

Use `-o`/`--output` to select the simulation output. A path ending in `/` is
treated as a directory and is created when missing; an existing directory uses
the standard reporter filenames. A path without a trailing separator is used
as the SQLite database filename override, including when the file does not yet
exist.

```sh
# Write the standard database file into a directory
./build/bin/malasim -i input.yml -o outputs/

# Write the SQLite output to this exact filename
./build/bin/malasim -i input.yml -o outputs/run.db
```
