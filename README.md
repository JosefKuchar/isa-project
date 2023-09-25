# TFTP Client & Server

Josef Kuchař (xkucha28), 25. 9. 2023

## Description

TODO

## How to run

### The client

`tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath`

- `-h` Server hostname
- `-p` Server port (default 69)
- `-f` Input file (default from standard input)
- `-t` Destination file path

#### Example

`tftp-client -p 1234 -h 127.0.0.1 -t test.txt -f tftp-client.cc`

### The server

`tftp-server [-p port] root_dirpath`

- `-p` Server port (default 69)
- `root_dirpath` File destination folder

#### Example

`tftp-server -p 1234 files`

## List of submitted files

- `client-args.cc`
- `client-args.h`
- `server-args.cc`
- `server-args.h`
- `packet-builder.cc`
- `packet-builder.h`
- `packet.cc`
- `packet.h`
- `enums.h`
- `tftp-client.cc`
- `tftp-server.cc`
- `Makefile`
