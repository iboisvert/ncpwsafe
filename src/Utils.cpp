/* Copyright 2022 Ian Boisvert */
#include "Utils.h"
#include <cstdlib>

void RandomizeFieldsBuffer(FIELD **fields)
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
        trashMemory(cbuf, rows * cols);
    }
}
