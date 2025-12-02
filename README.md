# discrub

discrub is a lightweight CLI tool for bulk Discord message management with customizable search queries.

## Usage

Build the project with:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The resulting executable is located at `build/discrub`.

### Authentication
Run `discrub` without arguments to authenticate. Credentials can be cached securely and reused in subsequent sessions.

## Development

### Build Instructions
Debug build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Release build (default):

```bash
cmake -S . -B build
cmake --build build
```

Quick rebuild:
```bash
cmake --build build
```

### Sanitizers
Enable sanitizers for debugging:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build
```

### Code Formatting
This project uses `clang-format` to maintain consistent style.

#### Format Files
```bash
cmake --build build --target format
```

#### Check Formatting without Overwriting
```bash
cmake --build build --target format-check
```

### Cleaning
Clean build artifacts:

```bash
cmake --build build --target clean
```

Full clean (recommended when switching configurations, sanitizer settings, or CMake options):

```bash
rm -rf build
```
