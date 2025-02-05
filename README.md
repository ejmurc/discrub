# Discrub

Discrub is a lightweight CLI tool for bulk Discord message deletion with customizable search queries.

![Discrub Screenshot](https://github.com/eliasmurcray/discrub/blob/mainline/preview.png?)

## Dependencies
Ensure the following dependencies are installed:

- [make](https://www.gnu.org/software/make/manual/make.html)
- [OpenSSL](https://wiki.openssl.org/index.php/Binaries)

## Configuration
The program requires a valid `options.json` file to run. Ensure the file exists and contains the necessary configuration.

### Required Options:
- **`server_id` (string)** – The ID of the server to search within.

### Optional Options:
- **`channel_id` (string)** – The ID of the channel to search within.
- **`include_nsfw` (boolean)** – Include or exclude NSFW content.
- **`content` (string)** – Search for messages containing specific text.
- **`mentions` (string)** – Filter messages mentioning a specific user or role.
- **`pinned` (boolean)** – Filter only pinned messages.
- **`delay_ms` (size_t)** – Initial delay (in milliseconds) between delete requests. This delay increases exponentially when hitting rate limits.
- **`max_id` (string)**    Search for messages before the specified max\_id.

### Example `options.json`:
```json
{
  "server_id": "123456789012345678",
  "channel_id": "987654321098765432",
  "include_nsfw": false,
  "content": "search keyword",
  "mentions": "username.asdf",
  "pinned": true,
  "delay_ms": 500,
  "max_id": "123456789012345678"
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

## Development

Contributions are welcome! If you're interested in contributing, please refer to [DEVELOPMENT.md](https://github.com/eliasmurcray/discrub/blob/mainline/DEVELOPMENT.md) for setup and contribution guidelines.

## Disclaimer
**Self-botting violates Discord’s Terms of Service and can result in account termination.** Use this script at your own discretion. The author assumes no responsibility for any consequences arising from its use.
