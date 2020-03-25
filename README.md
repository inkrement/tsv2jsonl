# tsv2jsonl

Minimalistic CLI-tool to convert MySQL/MariaDB dumps (TSV-Format) to line-wise json documents (e.g., jsonlines, ndjson). First you have to download and compile the source:

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