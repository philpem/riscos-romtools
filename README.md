# riscos-romtools
Tools for creating customised RISC OS 3 ROM images and "5th-column" Extension ROMs.

Phil Pemberton, philpem@gmail.com

If you do anything cool with this, please send me an email!

## Requirements

You will need the "srecord" tools.

The Python scripts require Python 3. `mkrom.py` also requires the Python 3 `yaml` module.


## Tools included

* RISC OS ROM tools
  * `checksum` (C program): Checks the RISC OS ROM checksum trailer (Additive sum and CRCs) and reports on its correctness.
  * `patchsum` (C program): Fixes the ROM header (ROM length word) and trailer (checksums).
  * `merge.sh`: Convert a 4-file emulator-format RISC OS ROM image into a single file.
  * `pad2mb.sh`, `pad4mb.sh`: Pad a ROM image to 4MB without fixing checksums
  * `stripe.sh`: Take a 2MB or 4MB ROM image and create a High and Low stripe image which may be programmed into a pair of M27C160-100 EPROMs.
* 5th-Column and Podule ROM tools
  * `podrom.c` (C program): Extract modules and data areas from a 5th Column or Podule ROM
  * `mkrom.py`: Creates a 5th Column ROM based on a YAML configuration file.
    * With minor changes, it could also be used to create Podule ROMs.


## Typical workflow (RISC OS ROM)

- Generate a modified RISC OS ROM using [qUE's tools and Flibble's tutorial](https://stardot.org.uk/forums/viewtopic.php?f=29&t=14164)
- Use `pad4mb.sh` to pad it to 4MB
- Use `patchsum` to fix the checksum and ROM size header
- Test it in Arculator
- Use `stripe.sh` to create images to program into 16-bit EPROMs
- Program some EPROMs and test them

4MB RISC OS ROM image generally don't work in ARM250-based machines. For reasons unknown, the ROM self test always fails.

When using a 4MB ROM, it will not be possible to use a POST Box to view the self-test output on any machine which uses `LA<21>` for the `TESTREQ` function. This includes the ARM250 machines, e.g. A3010, A3020 and A4000.


## Typical workflow (5th Column ROM)

- (Optional) Use `podrom` to extract the modules you need from a Podule or 5th Column ROM. (useful for Wizzo).
- Edit `config.yml` to list the modules you want in your final ROM.
- Run `mkrom.py` to build a 5th Column ROM.
- Assuming your 5th Column ROM is 64k in size:
  - Take a RISC OS 3.11 ROM dump
  - Append (2MB - 64k) of filler bytes (e.g. 0xFF)
  - Append your 5th Column ROM image
- Use `stripe.sh` to generate the striped images
- Program some EPROMs!


## EPROM compatibility

The RISC OS 3.11 ROM (2MB) may be programmed into a pair of STMicroelectronics M27C800-100 DIP-package 42-pin EPROMs.

If a custom ROM is created which requires a 4MB ROM, a pair of STMicroelectronics M27C160-100 DIP-package 42-pin EPROMs may be used.


