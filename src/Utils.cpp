/* Copyright 2022 Ian Boisvert */
#include "Utils.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Prevent memset from being optimized out
typedef void *(*memset_func)(void *, int, size_t);
volatile memset_func fmemset = memset;

void ZeroMemory(void *p, size_t len)
{
    fmemset(p, 0, len);
}

void ZeroString(std::string &str)
{
    ZeroMemory(str.data(), str.size());
    str.clear();
}

bool ReadPasswordFile(const std::string &pathname, std::string &password)
{
    std::FILE *f = std::fopen(pathname.c_str(), "rb");
    if (f == nullptr)
    {
        return false;
    }
    // Unbuffered, so that no copy of the password is left in a stdio buffer
    std::setvbuf(f, nullptr, _IONBF, 0);

    std::string result;
    // Most passwords fit without reallocating, which would leave copies on the heap
    result.reserve(256);
    char buf[256];
    bool eol = false;
    size_t n;
    while (!eol && (n = std::fread(buf, 1, sizeof(buf), f)) > 0)
    {
        if (const char *nl = static_cast<const char *>(std::memchr(buf, '\n', n)))
        {
            n = static_cast<size_t>(nl - buf);
            eol = true;
        }
        result.append(buf, n);
    }
    ZeroMemory(buf, sizeof(buf));

    bool ok = !std::ferror(f);
    int saved_errno = errno;
    std::fclose(f);

    if (ok)
    {
        if (!result.empty() && result.back() == '\r')
        {
            result.pop_back();
        }
        password.swap(result);
    }
    ZeroString(result);
    errno = saved_errno;
    return ok;
}

void ZeroFieldsBuffer(FIELD **fields)
{
    int rows, cols;
    char *cbuf;

    for (FIELD *field = *fields; field != nullptr; ++fields, field = *fields)
    {
        if (field_opts(field) & O_STATIC)
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

/** Expand environment variables in the string */
std::string ExpandEnvVars(const std::string &str)
{
    static const size_t npos = std::string::npos;

    std::string expanded;
    bool found_var = false;

    size_t last_end_pos = 0, start_pos = str.find("${");
    while (start_pos != npos)
    {
        found_var = true;

        size_t end_pos = str.find('}', start_pos+2);
        if (end_pos != npos)
        {
            expanded.append(str, last_end_pos, start_pos-last_end_pos);

            std::string env_var{str, start_pos+2, end_pos-(start_pos+2)};
            const char *value = getenv(env_var.c_str());
            if (value)
            {
                expanded.append(value);
            }

            last_end_pos = end_pos+1;
        }
        start_pos = str.find("${", end_pos);
    }

    if (found_var)
    {
        expanded.append(str, last_end_pos);
        return expanded;
    }
    else
    {
        return str;
    }
}