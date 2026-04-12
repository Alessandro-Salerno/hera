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

#include <hera/result.h>
#include <hera/types.h>

#define ER_MIN(x, y)        \
    ({                      \
        typeof(x) _x = (x); \
        typeof(y) _y = (y); \
        _x < _y ? _x : _y;  \
    })

#define ER_MAX(x, y)        \
    ({                      \
        typeof(x) _x = (x); \
        typeof(y) _y = (y); \
        _x < _y ? _y : _x;  \
    })

typedef ER_RESULT(ER_u64) ER_WCharLengthResult;
typedef ER_RESULT(ER_WChar) ER_WCharResult;

static inline ER_WCharLengthResult ER_char_width(ER_String s) {
    if (0 == s.str_len) {
        return (ER_WCharLengthResult){ER_STATUS_ERR, .res_err = "empty string"};
    }

    unsigned char c = (unsigned char)s.str_buf[0];

    if (0 == (c & 0x80)) {
        return (ER_WCharLengthResult){ER_STATUS_OK, .res_val = 1};
    } else if (0xC0 == (c & 0xE0)) {
        if (s.str_len < 2) {
            return (ER_WCharLengthResult){ER_STATUS_ERR,
                                          .res_err = "truncated utf8"};
        }
        return (ER_WCharLengthResult){ER_STATUS_OK, .res_val = 2};
    } else if (0xE0 == (c & 0xF0)) {
        if (s.str_len < 3) {
            return (ER_WCharLengthResult){ER_STATUS_ERR,
                                          .res_err = "truncated utf8"};
        }
        return (ER_WCharLengthResult){ER_STATUS_OK, .res_val = 3};
    } else if (0xF0 == (c & 0xF8)) {
        if (s.str_len < 4) {
            return (ER_WCharLengthResult){ER_STATUS_ERR,
                                          .res_err = "truncated utf8"};
        }
        return (ER_WCharLengthResult){ER_STATUS_OK, .res_val = 4};
    }

    return (ER_WCharLengthResult){ER_STATUS_ERR,
                                  .res_err = "invalid utf8 leading byte"};
}

static inline ER_WCharResult ER_char_to_wchar(ER_String s) {
    ER_WCharLengthResult w_res = ER_char_width(s);
    if (!ER_RESULT_OK(w_res)) {
        return (ER_WCharResult){ER_STATUS_ERR,
                                .res_err = ER_RESULT_ERROR(w_res)};
    }

    const unsigned char *buf = (void *)s.str_buf;
    ER_u64               len = ER_RESULT_GET(w_res);

    ER_WChar codepoint = 0;

    switch (len) {
        case 1:
            codepoint = buf[0];
            break;

        case 2:
            if (0x80 != (buf[1] & 0xC0))
                return (ER_WCharResult){ER_STATUS_ERR,
                                        .res_err = "invalid utf8 continuation"};
            codepoint = ((buf[0] & 0x1F) << 6) | (buf[1] & 0x3F);
            break;

        case 3:
            if (0x80 != (buf[1] & 0xC0) || 0x80 != (buf[2] & 0xC0))
                return (ER_WCharResult){ER_STATUS_ERR,
                                        .res_err = "invalid utf8 continuation"};
            codepoint = ((buf[0] & 0x0F) << 12) | ((buf[1] & 0x3F) << 6) |
                        (buf[2] & 0x3F);
            break;

        case 4:
            if (0x80 != (buf[1] & 0xC0) || 0x80 != (buf[2] & 0xC0) ||
                0x80 != (buf[3] & 0xC0))
                return (ER_WCharResult){ER_STATUS_ERR,
                                        .res_err = "invalid utf8 continuation"};
            codepoint = ((buf[0] & 0x07) << 18) | ((buf[1] & 0x3F) << 12) |
                        ((buf[2] & 0x3F) << 6) | (buf[3] & 0x3F);
            break;

        default:
            return (ER_WCharResult){ER_STATUS_ERR,
                                    .res_err = "unsupported utf8 length"};
    }

    return (ER_WCharResult){ER_STATUS_OK, .res_val = codepoint};
}

void ER_generic_panic_output(ER_String   input,    // input string/file contents
                             ER_u64      err_line, // human line number
                             ER_u64      err_col,  // human column number
                             ER_u64      err_lstart, // binary row start index
                             ER_u64      err_loff,   // binary column number
                             ER_u64      err_len, // length of the error segment
                             const char *fmt,     // error message printf format
                             ...);
