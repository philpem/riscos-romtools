# riscos-romtools
Tools for creating customised RISC OS 3 ROM images.

Phil Pemberton, philpem@gmail.com

## Requirements

You will need the "srecord" tools installed


## Tools included

* checksum (C program): Checks the RISC OS ROM checksum trailer (Additive sum and CRCs) and reports on its correctness.
* patchsum (C program): Fixes the ROM header (ROM length word) and trailer (checksums).
* merge.sh: Convert a 4-file emulator-format RISC OS ROM image into a single file.
* pad4mb.sh: Pad a ROM image to 4MB without fixing checksums
* stripe.sh: Take a 2MB or 4MB ROM image and create a High and Low stripe image which may be programmed into a pair of M27C160-100 EPROMs.


## Typical workflow

- Generate a ROM image
- Use `pad4mb.sh` to pad it to 4MB
- Use `patchsum` to fix the checksum and ROM size header
- Test it in Arculator
- Use `stripe.sh` to create images to program into 16-bit EPROMs
- Program some EPROMs and test them


## EPROM compatibility

The RISC OS 3.11 ROM (2MB) may be programmed into a pair of STMicroelectronics M27C800-100 DIP-package 42-pin EPROMs.

If a custom ROM is created which requires a 4MB ROM, a pair of STMicroelectronics M27C160-100 DIP-package 42-pin EPROMs may be used.

## Caveats

While the ARM250-based machines can boot a 4MB ROM, it will not be possible to use the Self-Test header (POST Box) to debug them. Attempting to do so will result in a ROM Checksum Error, however the machine will likely still boot (after blinking out the error code).
