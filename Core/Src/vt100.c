/*! \file vt100.c \brief VT100 terminal function library. */
//*****************************************************************************
//
// File Name	: 'vt100.c'
// Title		: VT100 terminal function library
// Author		: Pascal Stang - Copyright (C) 2002
// Created		: 2002.08.27
// Revised		: 2002.08.27
// Version		: 0.1
// Target MCU	: Atmel AVR Series
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include <stdint.h>
#include <stdio.h>

#include "cmdline.h"
#include "main.h"
#include "vt100.h"


// Program ROM constants

// Global variables

// Functions
void vt100Init(CliState_t *state)
{
  // initializes terminal to "power-on" settings
  // ESC c

 fprintf(state->strOut, "\x1B\x63");
}

void vt100ClearScreen(CliState_t *state)
{
  // ESC [ 2 J
  fprintf(state->strOut, "\x1B[2J");
}

void vt100SetAttr(uint8_t attr, CliState_t *state)
{
  // ESC [ Ps m
  fprintf(state->strOut, "\x1B[%dm",attr);
}

void vt100SetCursorMode(uint8_t visible, CliState_t *state)
{
  if(visible)
  // ESC [ ? 25 h
    fprintf(state->strOut, "\x1B[?25h");
  else
  // ESC [ ? 25 l
    fprintf(state->strOut, "\x1B[?25l");
}

void vt100SetCursorPos(uint8_t line, uint8_t col, CliState_t *state)
{
  // ESC [ Pl ; Pc H
  fprintf(state->strOut, "\x1B[%d;%dH",line,col);
}
