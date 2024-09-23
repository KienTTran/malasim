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