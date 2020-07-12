#!/bin/bash -xe

VER=$1

srec_cat ${VER} -Binary -Split 4 0 2 -Output ${VER}Low.bin -Binary
srec_cat ${VER} -Binary -Split 4 2 2 -Output ${VER}High.bin -Binary
