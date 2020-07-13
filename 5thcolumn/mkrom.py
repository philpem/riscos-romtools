#!/usr/bin/env python3
#
# Build a 5th-column ROM for Acorn systems

import struct
import yaml
from pprint import pprint

with open('config.yml') as yf:
    config = yaml.load(yf)

pprint(config)

# Start by allocating a bytearray to store the ROM we're building
romSizeBytes = config['romsize'] * 1024
romBuf = bytearray([0xFF]*romSizeBytes)

# Start filling in modules from the end of ROM, approaching the chunk directory
# We also need to reserve space for the 16-byte Extension ROM trailer
romAddr = romSizeBytes-16

# Start filling in the header and chunk directory from the start of ROM
romPtr = 0


def hdrAppend(byte):
    global romBuf, romPtr
    romBuf[romPtr] = byte
    romPtr += 1

def hdrAppendWord(word):
    hdrAppend(word & 0xFF)
    hdrAppend((word >> 8) & 0xFF)

def hdrAppendDword(dword):
    hdrAppendWord(dword & 0xFFFF)
    hdrAppendWord((dword >> 16) & 0xFFFF)


def addChunk(osid, subtype, data):
    global romBuf, romAddr

    # Size in bytes -- max 0xFFFFFF
    assert(len(data) < 0xFFFFFF)
    assert(len(data) > 0)

    # Allocate space for the data and the module length word
    # Ref: 5thColumn, "Note" re 32bit-wide ROMs.
    # https://gitlab.riscosopen.org/RiscOS/Sources/Kernel/-/blob/master/Docs/5thColumn/Manual
    romAddr -= len(data) + 4

    print("  at address ", hex(romAddr))

    # Make sure the module begins on a word boundary
    pad = 0
    if ((len(data) % 4) != 0):
        romAddr -= (4 - (len(data) % 4))
        pad = (4 - (len(data) % 4))

    print("  length     ", len(data), " (padding =", pad, "bytes)")

    # Make sure the chunk didn't collide with the chunk directory
    # 12 because one more chunk header + 4 byte terminator
    assert(romAddr > (romPtr+12))

    # Copy in the module length + 4
    mptr = len(data) + 4
    romBuf[romAddr+3] = (mptr >> 24) & 0xff
    romBuf[romAddr+2] = (mptr >> 16) & 0xff
    romBuf[romAddr+1] = (mptr >> 8)  & 0xff
    romBuf[romAddr+0] = (mptr)       & 0xff

    # Copy the data
    for i in range(len(data)):
        romBuf[romAddr+i+4] = data[i]

    # -- Create the chunk entry

    # OS identity byte and data length
    osib = 0x80 | ((osid & 7) << 4) | (subtype & 0xF)
    hdrAppendDword(osib | (len(data) << 8))
    # Start address
    hdrAppendDword(romAddr+4)



# Build the ECID
hdrAppend(0x00)     # Acorn-conformant, no FIQs, no IRQs
hdrAppend(0x03)     # Interrupt status and chunk directory follows, ROM is byte-wide
hdrAppend(0)        # Reserved
hdrAppendWord(config.get('product', 0x87))
hdrAppendWord(config.get('manufacturer', 0))
hdrAppend(config.get('country', 0))

# Add the interrupt status flags
hdrAppendDword(0)
hdrAppendDword(0)


# Now add the chunks
if 'manufacturerData' in config:
    md = config['manufacturerData']

    cfgMap = {
        'serial':      1,
        'mfgDate':     2,
        'modStatus':   3,
        'mfgPlace':    4,
        'description': 5,
        'partNumber':  6
        }

    for key,subtype in cfgMap.items():
        if key in md:
            print("adding chunk ", key)
            addChunk(7, subtype, bytes(md[key], 'latin1') + bytearray([0]))


# -- Modules --

for mfn in config['modules']:
    print("Adding module '%s'" % mfn)
    with open(mfn, 'rb') as f:
        data = f.read()
        addChunk(0, 1, data)

# -- Chunk Directory terminator --

# Append four bytes of zeroes to terminate the chunk directory
hdrAppendDword(0)


# -- ExtROM0 Trailer --

# Append the ROM length
romPtr = romSizeBytes - 16
hdrAppendDword(romSizeBytes)

# Append the trailing header
magic = b'ExtnROM0'
for i in range(len(magic)):
    romBuf[(romSizeBytes - 8) + i] = magic[i]

# Calculate the checksum
csum = sum([int.from_bytes(romBuf[i:i + 4], byteorder='little') for i in range(0, romSizeBytes-12, 4)])
hdrAppendDword(csum & 0xFFFFFFFF)

# Save to file
filename = config.get('filename', 'rom.bin')
with open(filename, 'wb') as f:
    f.write(romBuf)

