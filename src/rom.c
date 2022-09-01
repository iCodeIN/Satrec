//
// Satrec
// Copyright 2022 Wenting Zhang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "util.h"

uint8_t *rom;

void rom_init(uint8_t *rom_ptr) {
    rom = rom_ptr;
}

uint8_t *rom_get_ptr(size_t address) {
#if 0
    return &rom[address / 2];
#else
    return &rom[address];
#endif
}

void rom_write(size_t address, uint8_t value) {
    fatal("ROM write not implemented yet\n");
}

uint8_t rom_read(size_t address) {
#if 0
    uint8_t byte = rom[address / 2];
    uint8_t nibble = (address & 1) ? (byte >> 4) : (byte & 0xff);
#else
    uint8_t nibble = rom[address];
#endif
    return nibble;
}
