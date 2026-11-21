/* Copyright 2022 Ian Boisvert */
#include "config.h"
#include "Utils.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cwchar>
#include <cwctype>
#include <initializer_list>
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#include <strings.h>
#endif

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
#ifdef _WIN32
            // HOME is not normally set on Windows
            if (!value && env_var == "HOME")
            {
                value = getenv("USERPROFILE");
            }
#endif
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

size_t Utf8Length(std::string_view text)
{
    // Count the bytes that aren't UTF-8 continuation bytes
    return std::count_if(text.begin(), text.end(), [](char c) { return (c & 0xC0) != 0x80; });
}

/** Number of bytes in the first `n` characters of UTF-8 text */
static size_t Utf8PrefixBytes(std::string_view text, size_t n)
{
    size_t pos = 0;
    while (pos < text.size() && n > 0)
    {
        ++pos;
        while (pos < text.size() && (text[pos] & 0xC0) == 0x80) ++pos;
        --n;
    }
    return pos;
}

static void WrapParagraph(std::string_view para, size_t width, std::vector<std::string> &lines)
{
    const size_t first_line = lines.size();
    std::string line;
    size_t line_len = 0;
    size_t pos = 0;
    while (pos <= para.size())
    {
        size_t end = std::min(para.find(' ', pos), para.size());
        std::string_view word = para.substr(pos, end - pos);
        pos = end + 1;

        size_t word_len = Utf8Length(word);
        if (line_len > 0 && line_len + 1 + word_len > width)
        {
            lines.push_back(std::move(line));
            line.clear();
            line_len = 0;
        }
        if (line_len > 0)
        {
            line += ' ';
            ++line_len;
        }
        // Only a word longer than a whole line is broken
        while (word_len > width - line_len)
        {
            size_t n = width - line_len;
            size_t nbytes = Utf8PrefixBytes(word, n);
            line.append(word.substr(0, nbytes));
            lines.push_back(std::move(line));
            line.clear();
            line_len = 0;
            word.remove_prefix(nbytes);
            word_len -= n;
        }
        line.append(word);
        line_len += word_len;
    }
    if (line_len > 0 || lines.size() == first_line)
    {
        lines.push_back(std::move(line));
    }
}

std::vector<std::string> WrapText(const std::string &text, size_t width)
{
    width = std::max<size_t>(width, 1);
    std::vector<std::string> lines;
    std::string_view rest{text};
    while (!rest.empty())
    {
        size_t nl = rest.find('\n');
        WrapParagraph(rest.substr(0, nl), width, lines);
        if (nl == std::string_view::npos) break;
        rest.remove_prefix(nl + 1);
    }
    return lines;
}

/** `true` if the character set of the LC_CTYPE locale is UTF-8 */
static bool IsUtf8Ctype()
{
#ifdef HAVE_LANGINFO_H
    const char *codeset = nl_langinfo(CODESET);
    return codeset && (strcasecmp(codeset, "UTF-8") == 0 || strcasecmp(codeset, "utf8") == 0);
#else
    return false;
#endif
}

bool SetUtf8CtypeLocale()
{
    if (setlocale(LC_CTYPE, "") != nullptr && IsUtf8Ctype())
    {
        return true;
    }
    for (const char *name : {"C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8"})
    {
        if (setlocale(LC_CTYPE, name) != nullptr && IsUtf8Ctype())
        {
            return true;
        }
    }
    setlocale(LC_CTYPE, "");
    return false;
}

int ReadKey(WINDOW *win, wchar_t &wch)
{
    wint_t key;
    int rc = wget_wch(win, &key);
    if (rc == KEY_CODE_YES)
    {
        return static_cast<int>(key);
    }
    if (rc == CURSES_OK)
    {
        if (key < 0x80)
        {
            return static_cast<int>(key);
        }
        wch = static_cast<wchar_t>(key);
        return KEY_WIDE_CHAR;
    }
    return ERR;
}

std::string PrintableText(const char *text)
{
    std::string printable;
    std::mbstate_t state{};
    const char *p = text, *end = text + strlen(text);
    while (p < end)
    {
        wchar_t wc;
        size_t n = std::mbrtowc(&wc, p, end - p, &state);
        if (n == static_cast<size_t>(-1) || n == static_cast<size_t>(-2) || n == 0)
        {
            // Not a character of this character set: replace the byte, and any UTF-8
            // continuation bytes after it, which would be the rest of the character
            printable += '?';
            state = std::mbstate_t{};
            ++p;
            while (p < end && (*p & 0xC0) == 0x80) ++p;
            continue;
        }
        if (std::iswprint(wc))
        {
            printable.append(p, n);
        }
        else
        {
            printable += '?';
        }
        p += n;
    }
    return printable.empty() ? std::string("?") : printable;
}
