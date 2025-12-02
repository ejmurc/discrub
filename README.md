# discrub

discrub is a lightweight CLI tool for bulk Discord message management with customizable search queries.

## Usage

Build the project:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The resulting executable will be available at `build/discrub`.

### Authentication
Run `discrub` with no arguments to authenticate. Credentials can be securely cached and reused across sessions.

## Development

### Build Instructions
Debug build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Rebuild:
```bash
cmake --build build
```

### Sanitizers

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build
```

### Code Formatting
This project uses `clang-format` to enforce a consistent coding style.

Format all files:
```bash
cmake --build build --target format
```

Check formatting without modifying files:
```bash
cmake --build build --target format-check
```

### Cleaning
Clean build artifacts:

```bash
cmake --build build --target clean
```

Perform a full clean (recommended when switching build configurations, sanitizer settings, or CMake options):

```bash
rm -rf build
```
