# tsv2jsonl

Minimalistic CLI-tool to convert MySQL/MariaDB dumps (TSV-Format) to line-wise json documents (e.g., jsonlines, ndjson). 

```
tsv2jsonl [-h header_names] [-a] [in_file] [out_file]  
        
        -a autodetect types
        -h define column names (default is X1, X2, ...)
```

For example, we can trasform a mysql dump `./inc/example.tsv` to jsonl and store it in `./inc/example.jsonl`.

```sh
tsv2jsonl -a ./inc/example.tsv ./inc/example.jsonl
```

Columnnames automatically begin with a X and are followed by the colum number (X1, X2, X3). Alternatively, one can provide the header names of the tsv:

```
cat inc/example.tsv | tsv2jsonl -a -n id,hashtags,created_at,text,lang,retweets,coords,a_username,a_followers,a_friends,a_statuses,a_description,a_location,observation_date,geo,place,quotes,replies,favs,source
````

## Install

First you have to download and compile the source:

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
