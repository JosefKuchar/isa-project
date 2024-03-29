# TFTP Client a Server

Josef Kuchař (xkucha28), 20. 11. 2023

## Popis

Implementace TFTP klienta a serveru v jazyce C++. Vše je implementováno dle zadání a příslušných RFC. Podrobný popis je k dispozici v manual.pdf.

## Spuštění

### Klienta

`tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath`

- `-h` IP adresa/doménový název vzdáleného serveru
- `-p` port vzdáleného serveru, port vzdáleného serveru, pokud není specifikován je použit port 69
- `-f` cesta ke stahovanému souboru na serveru (download), pokud není specifikován používá se obsah stdin (upload)
- `-t` cesta, pod kterou bude soubor na vzdáleném serveru/lokálně uložen

#### Příklad

`./tftp-client -p 1234 -h 127.0.0.1 -t test.txt -f tftp-client.cc`

### Serveru

`tftp-server [-p port] root_dirpath`

- `-p` místní port, na kterém bude server očekávat příchozí spojení
- cesta k adresáři, pod kterým se budou ukládat příchozí soubory

#### Příklad

`./tftp-server -p 1234 files`

## Seznam odevzdaných souborů

- `src`
  - `client-args.cc`
  - `client-args.h`
  - `enums.h`
  - `packet-builder.cc`
  - `packet-builder.h`
  - `packet.cc`
  - `packet.h`
  - `server-args.cc`
  - `server-args.h`
  - `settings.h`
  - `tftp-client.cc`
  - `tftp-server.cc`
  - `utils.cc`
  - `utils.h`
- `Makefile`
- `README`
