# discrub

## Usage

Build the project:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The resulting executable will be available at `build/discrub`.

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

Enable AddressSanitizer and UndefinedBehaviorSanitizer:
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

Clean
```bash
cmake --build build --target clean
```

Full Clean
```bash
rm -rf build
```
