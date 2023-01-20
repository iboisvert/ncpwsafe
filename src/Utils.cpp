/* Copyright 2022 Ian Boisvert */
#include "Utils.h"
#include <cstdlib>

char *WideToMultibyteString(const wchar_t *src, char *dst, size_t dst_len)
{
    assert(src);
    assert(dst);
    __attribute__((unused)) size_t count = wcstombs(dst, src, dst_len);
    assert(count == dst_len-1);
    return dst;
}

char *WideToMultibyteString(const wchar_t *src, char **pdst)
{
    assert(src);
    assert(pdst);
    size_t mblen = wcstombs(NULL, src, 0) + 1;
    *pdst = new char[mblen];
    WideToMultibyteString(src, *pdst, mblen);
    return *pdst;
}

void WideToMultibyteString(const wchar_t *src, cstringT &dst)
{
    assert(src);
    size_t mblen = wcstombs(NULL, src, 0);
    dst.resize(mblen);
    WideToMultibyteString(src, dst.data(), mblen + 1);
}

extern StringX Utf8ToWideString(const char *src);

bool Utf8ToWideString(const char *src, unsigned src_len, StringX &dst)
{
    CUTF8Conv conv;
    return conv.FromUTF8(reinterpret_cast<const unsigned char *>(src), src_len, dst);
}

StringX Utf8ToWideString(const char *src)
{
    StringX dst;
    int len = strlen(src);
    Utf8ToWideString(src, len, dst);
    return dst;
}

bool Utf8ToWideString(const cstringT &src, StringX &dst)
{
    return Utf8ToWideString(src.c_str(), src.length(), dst);
}

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
