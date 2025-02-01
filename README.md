## Dependencies  
Ensure the following dependencies are installed:  

- [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)  
- [make](https://www.gnu.org/software/make/manual/make.html)  
- [OpenSSL](https://wiki.openssl.org/index.php/Binaries)  

### Optional  
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) (for code formatting)  

## Compilation  
Before building, create an `options.json` file with the following fields:  

```json
{
  "server_id": "<your_server_id>",
  "channel_id": "<your_channel_id>"
}
```  

Then, compile the project:  

```sh
make release
```  

The resulting binary will be located at `build/discrub`.  

## Usage  
Run the executable as needed:  

```sh
./build/discrub
```

## Disclaimer  
**Self-botting violates Discord’s Terms of Service and can result in account termination.** Use this script at your own discretion. The author assumes no responsibility for any consequences arising from its use.  
