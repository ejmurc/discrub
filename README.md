# Discrub

Discrub is an open source terminal client for mass deletion of messages on Discord.

![Discrub Screenshot](https://github.com/eliasmurcray/discrub/blob/mainline/preview.png)

## Dependencies
Ensure the following dependencies are installed:

- [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
- [make](https://www.gnu.org/software/make/manual/make.html)
- [OpenSSL](https://wiki.openssl.org/index.php/Binaries)

### Optional
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) (for code formatting)

## Configuration
The program requires a valid `options.json` file to run. Ensure the file exists and contains the necessary configuration.

### Required Options:
- **`server_id` (string)** – The ID of the server to search within.
- **`channel_id` (string)** – The ID of the channel to search within.

### Optional Options:
- **`include_nsfw` (boolean)** – Include or exclude NSFW content.
- **`content` (string)** – Search for messages containing specific text.
- **`mentions` (string)** – Filter messages mentioning a specific user or role.
- **`pinned` (boolean)** – Filter only pinned messages.
- **`delay_ms` (size_t)** – Initial delay (in milliseconds) between delete requests. This delay increases exponentially when hitting rate limits.

### Example `options.json`:
```json
{
  "server_id": "123456789012345678",
  "channel_id": "987654321098765432",
  "include_nsfw": false,
  "content": "search keyword",
  "mentions": "username.asdf",
  "pinned": true,
  "delay_ms": 500
}
```

## Compilation
Once `options.json` is set up, build the project with:

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
