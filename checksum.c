/**
 * Checksum -- a tool to check RISC OS 3.1x ROM headers and footers.
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

// Number of bytes CRC'd
static int crc_i = 0;

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
	if (argc == 1) {
		fprintf(stderr, "syntax: checksum romfile\n");
		return -1;
	}

	FILE *fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		fprintf(stderr, "cannot open input file\n");
		return -1;
	}

	make_crc_table();

	// get the filesize
	fseek(fp, 0, SEEK_END);
	size_t romsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// ignore the last 3x 32-bit words in ROM
	romsize -= (3*4);

	// checksum time!

	uint32_t csum = 0;
	uint8_t bytes[4];

	while (romsize > 0) {
		romsize -= 4;
		fread(&bytes, 1, 4, fp);
		csum +=
			((uint32_t)bytes[0])   |
			((uint32_t)bytes[1] << 8)   |
			((uint32_t)bytes[2] << 16)    |
			((uint32_t)bytes[3] << 24);

		// update the CRC too
		update_crc_quad(bytes);
	}

	printf("sum accum: %08X\n", csum);

	// calculate the expected checksum
	printf("expected : %08X\n", 0-csum);

	// read the additive checksum from the file
	fread(&bytes, 1, 4, fp);
	uint32_t read_csum =
		((uint32_t)bytes[0])   |
		((uint32_t)bytes[1] << 8)   |
		((uint32_t)bytes[2] << 16)    |
		((uint32_t)bytes[3] << 24);

	printf("read csum: %08X\n", read_csum);

	// update the CRC
	update_crc_quad(bytes);

	// check the checksum
	csum += read_csum;

	if (csum != 0) {
		printf("checksum is incorrect\n");
	} else {
		printf("checksum is OKAY!\n");
	}

	printf("CRC:\n");
	for (int i=0; i<4; i++) {
		printf("  crc[%d] = 0x%04X\n", i, crc_acc[i]);
	}

	// Applying the CRC to the CRC bytes should zero the accumulator
	for (int i=0; i<2; i++) {
		fread(&bytes, 1, 4, fp);
		update_crc_quad(bytes);
	}

	if ((crc_acc[0] | crc_acc[1] | crc_acc[2] | crc_acc[3]) == 0) {
		printf("--> CRC good!\n");
	} else {
		printf("--> CRC bad :(\n");
	}


	return 0;
}
