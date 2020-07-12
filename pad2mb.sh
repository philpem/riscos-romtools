#!/bin/sh

ROMSIZE=2097152

srec_cat $1 -Binary -fill 0xFF 0 $ROMSIZE -o $1.extended -Binary
