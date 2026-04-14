/*
BSD 2-Clause License

Copyright (c) 2026, Alessandro Salerno and contributors

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

#define ER_RESULT(type)                          \
    struct {                                     \
        ER_Status res_status;                    \
        union {                                  \
            type        res_val;                 \
            const char *res_err;                 \
            void       *res_panicarg;            \
        };                                       \
        ER_ResultPanicHandler *res_panichandler; \
    }

typedef void ER_ResultPanicHandler(void *arg);

#define ER_RESULT_UNWRAP(result)                     \
    ER_result_unwrap_impl((result).res_status,       \
                          &(result).res_val,         \
                          (result).res_err,          \
                          (result).res_panichandler, \
                          (result).res_panicarg)

#define ER_RESULT_STATUS(result) ((result).res_status)
#define ER_RESULT_OK(result)     (ER_RESULT_STATUS(result) == ER_STATUS_OK)
#define ER_RESULT_GET(result)    ((result).res_val)
#define ER_RESULT_ERROR(result)  ((result).res_err)

#define ER_RESULT_INTERNAL_HELP1(x) #x
#define ER_RESULT_INTERNAL_HELP(x)  ER_RESULT_INTERNAL_HELP1(x)

#define ER_RESULT_CAST(t, r)                                        \
    ((r).res_status == ER_STATUS_ERR                                \
         ? (t){.res_status = ER_STATUS_ERR, .res_err = (r).res_err} \
     : (r).res_status == ER_STATUS_PANIC                            \
         ? (t){.res_status       = ER_STATUS_PANIC,                 \
               .res_panicarg     = (r).res_panicarg,                \
               .res_panichandler = (r).res_panichandler}            \
         : (t){.res_status = ER_STATUS_ERR,                         \
               .res_err    = "invalid result cast at " __FILE__     \
                          ":" ER_RESULT_INTERNAL_HELP(__LINE__)})

typedef enum ER_Status {
    ER_STATUS_OK = 0,
    ER_STATUS_ERR,
    ER_STATUS_PANIC
} ER_Status;

void *ER_result_unwrap_impl(ER_Status              status,
                            void                  *val,
                            const char            *message,
                            ER_ResultPanicHandler *panic_handler,
                            void                  *panic_arg);
