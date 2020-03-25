# tsv2jsonl

Minimalistic CLI-tool to convert MySQL/MariaDB dumps (TSV-Format) to line-wise json documents (e.g., jsonlines, ndjson). First you have to download and compile the source:


> tsv2jsonl [-h header_names] [-a] [in_file] [out_file]
>        header_names X1,X2,X3,..
>        -a autodetect types

For example, we can trasform a mysql dump `./inc/example.tsv` to jsonl and store it in `./inc/example.jsonl`.

```sh
tsv2jsonl -a ./inc/example.tsv ./inc/example.jsonl
```

## Install

```sh
git clone https://github.com/inkrement/tsv2jsonl.git
cd tsv2jsonl && make
```

With the following command you can install it on your system:

```sh
sudo make install
```

## Requirements
 - C++17 copiler & build tools (i.e., make)
 - should work on most POSIX compatible systems