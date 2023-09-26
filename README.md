# huffman_toy

### Compiling
```
./build.sh
```

### Compressing
```
./huffman -e -i /bin/uname -o /tmp/uname.compressed
Input file:  /bin/uname
Output file: /tmp/uname.compressed

Header on compress
header->bitmap:                       1
header->orig_size_max_bytes:          2
header->orig_size:                39288
header->freq_max_bits:               15
header->nodes_count:                256

Original   size: 39288
Compressed size: 24457
Output file is 62.25% of input file (37.75% smaller)
```

### Decompressing
```
./huffman -d -i /tmp/uname.compressed -o /tmp/uname
Input file:  /tmp/uname.compressed
Output file: /tmp/uname

Header on decompress
header->bitmap:                       1
header->orig_size_max_bytes:          2
header->orig_size:                39288
header->freq_max_bits:               15
header->nodes_count:                256

Original   size: 39288
Compressed size: 24457
researcher@ubuntu:~/Code/c/huffman$ chmod +x /tmp/uname
researcher@ubuntu:~/Code/c/huffman$ /tmp/uname
Linux
```
