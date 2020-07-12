/**
 * Patchsum -- a tool to fix RISC OS 3.1x ROM headers and footers.
 *
 * Based on Bigsplit2 and CRC by Acorn, released by RISC OS Open Ltd:
 * https://gitlab.riscosopen.org/RiscOS/Utilities/Release/bigsplit2
 */

#include <stdio.h>
#include <stdint.h>

/* A fundamental constant of the CRC algorithm */
#define CRC_MAGIC 0xA001

/* I don't think you'll want to change this one */
#define CRC_TABLE_SIZE (1<<8)

/* CRC table to speed up CRC calculation */
static uint16_t crc_table[CRC_TABLE_SIZE];

/* CRC accumulator */
static uint16_t crc_acc[4] = {0,0,0,0};

/// Create CRC table, must be called at startup
static void make_crc_table(void) {
  int i,j;
  uint16_t crc;
  for(i=0;i<CRC_TABLE_SIZE;i++) {
    crc=i;
    for(j=0;j<8;j++)
      crc=(crc>>1)^(crc&1?CRC_MAGIC:0);
    crc_table[i]=crc;
  };
}

// Update the CRC with a 4-byte quad
static void update_crc_quad(const uint8_t *quad)
{
	for (int i=0; i<4; i++) {
		crc_acc[i] ^= (quad[i] & 0xFF);
		crc_acc[i] = (crc_acc[i] >> 8) ^ crc_table[crc_acc[i] & 0xff];
	}
}




int main(int argc, char **argv)
{
	uint32_t csum = 0;
	uint8_t bytes[4];
	FILE *fp;

	if (argc == 1) {
		fprintf(stderr, "syntax: patchsum romfile\n");
		return -1;
	}

	fp = fopen(argv[1], "r+b");
	if (fp == NULL) {
		fprintf(stderr, "cannot open input file\n");
		return -1;
	}

	make_crc_table();

	// get the filesize
	fseek(fp, 0, SEEK_END);
	size_t romsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//
	// The RISC OS ROM starts with a header which looks like this:
	//
	//   * 4-byte jump to startup code
	//   * 4-byte ROM length (little endian)
	//   * 4-byte pointer to code vector table
	//   * 4-byte pointer to data vector table
	//   * 4-byte pointer to branch table
	//   * Three unused words which encode branches to the start of ROM.
	//
	// The vector tables are used to allow downloaded selftest code to use the ROM selftest subroutines.
	//
	// We need to patch the ROM length or the selftest will miscalculate the ROM checksum and fail to boot.
	//
	// Source: https://gitlab.riscosopen.org/RiscOS/Sources/Kernel/-/blob/RO_3_60/OldTestSrc/Begin#L149
	//

	// patch ts_Rom_length
	printf("Setting ts_Rom_length to 0x%08lX (%lu bytes = %lu MiB)\n",
			romsize, romsize, romsize / 1048576);
	fseek(fp, 4, SEEK_SET);
	bytes[0] = romsize         & 0xFF;
	bytes[1] = (romsize >> 8)  & 0xFF;
	bytes[2] = (romsize >> 16) & 0xFF;
	bytes[3] = (romsize >> 24) & 0xFF;
	fwrite(&bytes, 1, 4, fp);

	/////////////////
	// Checksum recalculation
	/////////////////

	// back to the start again...
	fseek(fp, 0, SEEK_SET);

	// ignore the last 3x 32-bit words in ROM
	romsize -= (3*4);

	// checksum time!
	while (romsize > 0) {
		romsize -= 4;
		fread(&bytes, 1, 4, fp);
		csum +=
			((uint32_t)bytes[0])        |
			((uint32_t)bytes[1] << 8)   |
			((uint32_t)bytes[2] << 16)  |
			((uint32_t)bytes[3] << 24);

		// update the CRC too
		update_crc_quad(bytes);
	}

	// calculate the expected checksum
	csum = 0-csum;

	// convert to bytes and write
	printf("Patching additive checksum...\n");
	bytes[0] = csum         & 0xFF;
	bytes[1] = (csum >> 8)  & 0xFF;
	bytes[2] = (csum >> 16) & 0xFF;
	bytes[3] = (csum >> 24) & 0xFF;
	fwrite(&bytes, 1, 4, fp);

	// update the CRC with the additive checksum
	update_crc_quad(bytes);

	// status message and print the CRC
	printf("Patching CRC...\n");
	for (int i=0; i<4; i++) {
		printf("  crc[%d] = 0x%04X\n", i, crc_acc[i]);
	}

	// patch the CRC
	for (int i=0; i<4; i++) {
		bytes[i] = crc_acc[i] & 0xFF;
	}
	fwrite(&bytes, 1, 4, fp);

	for (int i=0; i<4; i++) {
		bytes[i] = (crc_acc[i] >> 8) & 0xFF;
	}
	fwrite(&bytes, 1, 4, fp);


	// and... done!
	fclose(fp);

	return 0;
}
