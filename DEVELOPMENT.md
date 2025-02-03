# Contributing to Discrub  

Contributions are welcome! If you find a bug or issue you can fix, follow these steps:  

1. **Fork the repository** and create a new branch for your changes.  
2. **Make your changes** and ensure they follow the project's coding standards.  
3. **Run formatting checks** before submitting your PR (see below).  
4. **Create a pull request** with a clear description of your changes.  

## Building  

Ensure the following dependencies are installed:  

- [make](https://www.gnu.org/software/make/manual/make.html)  
- [OpenSSL](https://wiki.openssl.org/index.php/Binaries)  
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html)  

### Build Process  

To compile the project for development, run:  

```sh
make dev
```  

The resulting executable will be located at `build/discrub`. You can run it with:  

```sh
./build/discrub
```  

### Code Formatting  

Before submitting a pull request, format the source and header files by running:  

```sh
make format
```  

This ensures consistent code style across the project.  
