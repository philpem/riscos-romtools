#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PRESENTSTR(x) (x ? "present" : "not present")
#define WORD16(b,o) (b[o] | (b[o+1] << 8))
#define WORD24(b,o) (WORD16(b,o) | (b[o+2] << 16))
#define WORD32(b,o) (WORD16(b,o) | (WORD16(b,o+2) << 16))

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "syntax: %s filename\n", argv[0]);
		return -1;
	}


	FILE *f = fopen(argv[1], "rb");
	if (f == NULL) {
		fprintf(stderr, "error opening '%s'\n", argv[1]);
		return -2;
	}


	// read file into buffer
	uint8_t buf[65536];
	size_t nbytes = fread(&buf, 1, sizeof(buf), f);
	fclose(f);

	// check ACORN bit in ECID
	if (buf[0] & 0x80) {
		fprintf(stderr, "Not an Acorn-conformant ECID\n");
		return -3;
	}
	if (buf[0] & 0x7A) {
		fprintf(stderr, "Reserved bits in ECID[0] not zero\n");
		return -4;
	}
	if (buf[1] & 0xF0) {
		fprintf(stderr, "Reserved bits in ECID[1] not zero\n");
		return -5;
	}
	if (buf[2] != 0) {
		fprintf(stderr, "Reserved bits in ECID[2] not zero\n");
		return -6;
	}

	printf("%zu bytes read\n", nbytes);

	// --- ECID header ----
	printf("--- EXPANSION CARD IDENTITY ---\n");
	printf("  ID byte          %02X -- %s%s%s\n", buf[0],
			buf[0]&0x80 ? "" : "Acorn-conformant ",
			buf[0]&0x04 ? "w/FIQ " : "no FIQ ",
			buf[0]&0x01 ? "w/IRQ " : "no IRQ "
			);
	bool hasChunkDir = buf[1] & 1;
	printf("  Chunk directory  %s\n", PRESENTSTR(hasChunkDir));
	bool hasIntPtrs  = buf[1] & 2;
	printf("  Interrupt ptrs   %s\n", PRESENTSTR(hasIntPtrs));
	printf("  Bus width        %d bits\n", 1 << (3 + ((buf[1] >> 2) & 3)));
	printf("  Product ID       &%04X\n", WORD16(buf, 3));
	printf("  Manufacturer ID  &%04X\n", WORD16(buf, 5));
	printf("  Country code     &%02X\n", buf[7]);

	size_t n = 8;
	if (hasIntPtrs) {
		printf("  IRQ Bitmask      &%02X\n", buf[n++]);
		printf("  IRQ address      &%06X\n", WORD24(buf,n));  n+=3;
		printf("  FIQ Bitmask      &%02X\n", buf[n++]);
		printf("  FIQ address      &%06X\n", WORD24(buf,n));  n+=3;
	} else {
		printf("  (no interrupt pointer block present)\n");
	}

	printf("\n");


	size_t nch = 1;
	if (hasChunkDir) {
		printf("--- CHUNK DIRECTORY ---\n");

		// -- CHUNK DIRECTORY --
		while (n < nbytes) {

			uint8_t osid = buf[n++];
			uint32_t size = WORD24(buf,n); n+=3;
			uint32_t addr = WORD32(buf,n); n+=4;

			if ((osid == 0) && (size == 0)) {
				printf("End of chunk directory reached.\n");
				break;
			}

			printf("Chunk %lu:\n", nch);
			printf("  Offset &%X, %u bytes (ends at &%X)\n", addr, size, addr+size);

			{
				char foo[32];
				snprintf(foo, sizeof(foo), "%02lu.bin", nch);
				FILE *o = fopen(foo, "wb");
				fwrite(&buf[addr], 1, size, o);
				fclose(o);
				printf("  Saved as '%s'\n", foo);
			}

			// validate OSID
			if (!(osid & 0x80)) {
				fprintf(stderr, "ERROR: OSID MSB not set, ofs=&%lX\n", n);
				return -7;
			}

			switch((osid >> 4) & 0x7) {
				case 0:
					// Acorn OS 0, Arthur/RISC OS
					printf("  Type %d: Acorn Arthur/RISC OS, ", (osid >> 4) & 7);
					switch(osid & 0x0F) {
						case 0:
							printf("Loader\n");
							break;
						case 1:
							printf("Relocatable Module");
							// try to find the module name
							if (WORD32(buf,addr+0x10) != 0) {
								printf(": %s\n", &buf[addr+WORD32(buf,addr+0x10)]);
							}
							if (WORD32(buf,addr+0x14) != 0) {
								printf("    %s\n", &buf[addr+WORD32(buf,addr+0x14)]);
							}
							break;
						case 2:
							printf("BBC ROM\n");
							break;
						case 3:
							printf("Sprite\n");
							break;
						default:
							printf("Reserved type %d\n", osid&0xF);
					}
					break;

				case 2:
					printf("  Type %d: UNIX, ", (osid >> 4) & 7);
					switch(osid & 0x0F) {
						case 0:
							printf("Loader");
							break;
						default:
							printf("Reserved type %d", osid&0xF);
					}
					printf("\n");
					break;

				case 6:
					printf("  Type %d Manufacturer-defined data, subtype %u\n", (osid>>4)&7, osid&0xF);
					printf("\n");
					break;

				case 7:
					printf("  Type %d Device data, subtype %u:\n", (osid>>4)&7, osid&0xF);
					switch(osid&0xF) {
						case 0:
							printf("    Link to another chunk directory, addr=&%X\n", addr); break;
						case 1:
							printf("    Serial number:        '%s'\n", &buf[addr]); break;
						case 2:
							printf("    Date of Manufacture:  '%s'\n", &buf[addr]); break;
						case 3:
							printf("    Modification status:  '%s'\n", &buf[addr]); break;
						case 4:
							printf("    Place of manufacture: '%s'\n", &buf[addr]); break;
						case 5:
							printf("    Description:         '%s'\n", &buf[addr]); break;
						case 6:
							printf("    Part number:         '%s'\n", &buf[addr]); break;
						case 7:
							printf("    Ethernet MAC addr.:  ");
							for (size_t i=0; i<6; i++) {
								if (i < 5) {
									printf("%02X-", buf[addr+i]);
								} else {
									printf("%02X\n", buf[addr+i]);
								}
							}
							break;
						case 8:
							printf("    PCB revision:        %u\n", WORD32(buf, addr)); break;
						case 15:
							printf("    (Empty chunk)\n"); break;

						default:
							printf("    Reserved\n");
					}
					break;

				default:
					printf("    Type %d: Reserved, subtype %u", (osid>>4)&7, osid&0xF);
					break;
			}
			printf("\n");

			// increment chunk number
			nch++;

		}
	}

	printf("\n");

	// Scan for extension headers
	printf("--- Extension ROM headers ---\n");
	for (size_t i=0; i<nbytes; i += 16384) {
		if (memcmp(&buf[i+0x3FF8], "ExtnROM0", 8) == 0) {
			printf("  Header seen at &%lX\n", i+0x3FF0);
			uint32_t romSize = WORD32(buf, i+0x3FF0);
			printf("    ROM size %d bytes\n", romSize);
			uint32_t romCsum = WORD32(buf, i+0x3FF4);
			printf("    Checksum &%08X\n", romCsum);

			// check the checksum
			uint32_t cs = 0;
			for (size_t m=0; m<romSize-12; m+=4) {
				cs += WORD32(buf, m);
			}
			if (cs == romCsum) {
				printf("      OK\n");
			} else {
				printf("      BAD: ROM CS &%08X, calculated &%08X\n", romCsum, cs);
			}
		}
	}

	return 0;
}
