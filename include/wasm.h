/*******************************************************************************
* MIT License
*
* Copyright (c) 2025 Curtis McCoy
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef WASP_WASM_DEFS_
#define WASP_WASM_DEFS_

#include "types.h"

#include "str.h"

#ifdef __WASM__
// allgedly supposed to work, but doesn't
//#define export __attribute__((used))

// also supposed to work, but doesn't
//#define export __attribute__((visibility( "default" )))

// very clunky, but actually does work
# define export(fn_name) __attribute__((export_name(#fn_name))) fn_name

typedef enum {
  CONCOL_Black   = 0x000000,
  CONCOL_Red     = 0xff0000,
  CONCOL_Green   = 0x00ff00,
  CONCOL_Yellow  = 0xffff00,
  CONCOL_Blue    = 0x0000ff,
  CONCOL_Purple  = 0xff00ff,
  CONCOL_Cyan    = 0x00ffff,
  CONCOL_White   = 0xffffff,
  CONCOL_bBlack  = 0x1000000,
  CONCOL_bRed    = 0x1ff0000,
  CONCOL_bGreen  = 0x100ff00,
  CONCOL_bYellow = 0x1ffff00,
  CONCOL_bBlue   = 0x10000ff,
  CONCOL_bPurple = 0x1ff00ff,
  CONCOL_bCyan   = 0x100ffff,
  CONCOL_bWhite  = 0x1ffffff,
} ConsoleColor;

void wasm_write(slice_t slice);
void wasm_write_color(slice_t slice, ConsoleColor color);
void wasm_alert(slice_t slice);

// ndef __WASM__
#else
# define export(fn_name) fn_name

typedef enum {
  CONCOL_Black   = 30, // \033[0;30m
  CONCOL_Red     = 31, // \033[0;31m
  CONCOL_Green   = 32, // \033[0;32m
  CONCOL_Yellow  = 33, // \033[0;33m
  CONCOL_Blue    = 34, // \033[0;34m
  CONCOL_Purple  = 35, // \033[0;35m
  CONCOL_Cyan    = 36, // \033[0;36m
  CONCOL_White   = 37, // \033[0;37m
  CONCOL_bBlack  = 40, // \033[1;30m
  CONCOL_bRed    = 41, // \033[1;31m
  CONCOL_bGreen  = 42, // \033[1;32m
  CONCOL_bYellow = 43, // \033[1;33m
  CONCOL_bBlue   = 44, // \033[1;34m
  CONCOL_bPurple = 45, // \033[1;35m
  CONCOL_bCyan   = 46, // \033[1;36m
  CONCOL_bWhite  = 47, // \033[1;37m
} ConsoleColor;

#endif

#endif
