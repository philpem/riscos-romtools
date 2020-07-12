.PHONY: all

all: checksum patchsum

checksum: checksum.c
	$(CC) -o $@ $<

patchsum: patchsum.c
	$(CC) -o $@ $<

