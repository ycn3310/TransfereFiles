# transfer

A small command-line tool written in C for sending a single file from one Windows machine to another over TCP, using raw Winsock2. It shows a live progress bar with transfer speed and ETA on both ends.

![Client/server flow](https://github.com/ycn3310/TransfereFiles/blob/main/client_server_file_transfer_flow.png)

## Features

- Send or receive a file with a single executable (`-s` / `-r`)
- Length-prefixed protocol: the filename and file size are sent before the data
- Handles partial `send()` / `recv()` calls with `sendAll()` / `recvAll()` helpers
- Supports files larger than 4 GB (64-bit sizes, `_fseeki64` / `_ftelli64`)
- Live progress bar with MB/s, Mb/s and ETA
- `TCP_NODELAY` enabled on the sockets
- Basic sanity check on the received filename length

## Project structure

```
.
├── main.c      # argument parsing, WSAStartup, dispatch to send / receive
├── send.c/.h   # sender: connect, send header, stream file
├── recv.c/.h   # receiver: bind, listen, accept, receive header, write file
├── common.c/.h # progress bar and shared buffer-size constants
└── docs/
    └── flow.png
```

## Requirements

- Windows (uses Winsock2 and the Win32 API)
- A C compiler with the Windows SDK / Winsock headers, e.g. [MinGW-w64](https://www.mingw-w64.org/) (GCC) or MSVC

## Build

**MinGW-w64 (GCC):**

```bash
gcc main.c send.c recv.c common.c -o transfer.exe -lws2_32
```

**MSVC (Developer Command Prompt):**

```bat
cl main.c send.c recv.c common.c /Fe:transfer.exe ws2_32.lib
```

## Usage

```
transfer -s|-r -p <port> [-ip <address>] [-f <file>] [-d <directory>]
```

| Flag | Mode | Description |
|------|------|-------------|
| `-s` | – | Send mode |
| `-r` | – | Receive mode |
| `-p <port>` | both | Port to connect to (sender) or listen on (receiver) |
| `-ip <address>` | send | IP address of the receiver |
| `-f <path>` | send | Path of the file to send |
| `-d <dir>` | receive | Directory where the received file is saved |
| `-h` | – | Show help |

### Example

1. Start the receiver first on the destination machine:

   ```bat
   transfer.exe -r -p 8888 -d C:\Users\me\Downloads
   ```

2. Then run the sender on the source machine:

   ```bat
   transfer.exe -s -ip 192.168.1.20 -p 8888 -f C:\Users\me\Documents\archive.zip
   ```

For a local test on a single machine, use `-ip 127.0.0.1` in two terminals.

> Make sure the chosen port is allowed through the receiver's firewall.

## How it works

The sender and receiver follow the flow shown above:

1. **Sender**: `WSAStartup` → `getaddrinfo` + `socket` → `connect` → send file → `closesocket`
2. **Receiver**: `WSAStartup` → `getaddrinfo` + `bind` → `listen` → `accept` → receive file → `closesocket`

### Wire protocol

After the TCP connection is established, the sender transmits:

| Field | Size | Encoding |
|-------|------|----------|
| Filename length | 4 bytes | `uint32_t`, network byte order |
| Filename | N bytes | raw bytes, no null terminator |
| File size | 8 bytes | `uint64_t`, host byte order (little-endian on x86) |
| File data | file size bytes | sent in 1 MB chunks |

The receiver reads the header, creates `<directory>\<filename>`, then reads until the sender closes the connection.

## Configuration

Chunk sizes are defined in `common.h`:

```c
#define BYTES_TO_SEND    1*1024*1024
#define BYTES_TO_RECEIVE 1*1024*1024
```

## Limitations

- Windows only
- One file per run, one client per run
- IPv4 only
- No encryption or authentication: use it on trusted networks only
- No integrity check (checksum) after transfer
- No resume support for interrupted transfers
- The file size field is not converted to network byte order, so both machines must share the same endianness (true for all x86 / x64 Windows machines)

## Roadmap ideas

- Checksum verification (e.g. CRC32 or SHA-256)
- Resume interrupted transfers
- Multiple files / directories
- IPv6 support
- Cross-platform support (POSIX sockets)

## License

Add a license of your choice (e.g. MIT) in a `LICENSE` file.
