/*
BSD 2-Clause License

Copyright (c) 2026, Alessandro Salerno

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <hera/graph.h>

#define ER_SVG_WIDTH       1600
#define ER_SVG_HEIGHT      1600
#define ER_SVG_FONT_SIZE_L 14
#define ER_SVG_FONT_SIZE_M 12
#define ER_SVG_FONT_SIZE_S 10

#define ER_SVG_ENTITY_W_PAD 40
#define ER_SVG_ENTITY_H_MIN 60
#define ER_SVG_ATTR_H_STEP  22

#define ER_SVG_RELATION_W_PAD 60
#define ER_SVG_RELATION_H_MIN 60

#define ER_SVG_GRID_X_STEP 150
#define ER_SVG_GRID_Y_STEP 150

#define ER_SVG_GEN_MERGE_Y_OFF 35

#define ER_SVG_KEY_ATTR_COLOR "blue"
#define ER_SVG_CARD_COLOR     "darkred"

void ER_svg_emit(ER_Graph *graph);
