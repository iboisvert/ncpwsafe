/* Copyright 2022 Ian Boisvert */
#include "Utils.h"
#include <cstdlib>
#include <cstring>

// Prevent memset from being optimized out
typedef void *(*memset_func)(void *, int, size_t);
volatile memset_func fmemset = memset;

void ZeroMemory(void *p, size_t len)
{
    fmemset(p, 0, len);
}

void ZeroFieldsBuffer(FIELD **fields)
{
    int rows, cols;
    char *cbuf;

    for (FIELD *field = *fields; field != nullptr; ++fields, field = *fields)
    {
        if (field_opts(field) && O_STATIC)
        {
            field_info(field, &rows, &cols, nullptr, nullptr, nullptr, nullptr);
        }
        else
        {
            dynamic_field_info(field, &rows, &cols, nullptr);
        }
        cbuf = field_buffer(field, 0);
        ZeroMemory(cbuf, rows * cols);
    }
}
