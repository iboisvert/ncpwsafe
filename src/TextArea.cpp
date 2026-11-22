/* Copyright 2026 Ian Boisvert */
#include "TextArea.h"
#include <algorithm>
#include <cwchar>
#include "Utils.h"

/** Bytes in the character at `pos` of `s`: 1 for a byte that isn't a character */
static size_t CharLen(std::string_view s, size_t pos)
{
    std::mbstate_t state{};
    wchar_t wc;
    const size_t n = std::mbrtowc(&wc, s.data() + pos, s.size() - pos, &state);
    if (n == static_cast<size_t>(-1) || n == static_cast<size_t>(-2) || n == 0)
    {
        return 1;
    }
    return n;
}

/** The start of the character before `pos` of `s` */
static size_t PrevPos(std::string_view s, size_t pos)
{
    size_t p = 0, prev = 0;
    while (p < pos)
    {
        prev = p;
        p += CharLen(s, p);
    }
    return prev;
}

/** A character as it is shown: a tab as a space, a character that can't be shown as '?' */
static std::string Shown(std::string_view ch)
{
    if (ch == "\t")
    {
        return " ";
    }
    return PrintableText(std::string(ch).c_str());
}

/** Columns taken by the character at `pos` of `s`, `len` bytes long, as it is shown */
static int CharWidth(std::string_view s, size_t pos, size_t len)
{
    return DisplayWidth(Shown(s.substr(pos, len)));
}

TextArea::TextArea(std::string_view text)
{
    SetText(text);
}

TextArea::~TextArea()
{
    for (std::string &line : lines_)
    {
        ZeroMemory(line.data(), line.size());
    }
}

void TextArea::SetText(std::string_view text)
{
    Clear();
    lines_.clear();
    breaks_.clear();
    new_break_.clear();
    std::string line;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const char c = text[i];
        if (c != '\r' && c != '\n')
        {
            line += c;
            continue;
        }
        std::string line_break(1, c);
        if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
        {
            line_break = "\r\n";
            ++i;
        }
        if (new_break_.empty())
        {
            new_break_ = line_break;
        }
        lines_.push_back(std::move(line));
        line.clear();
        breaks_.push_back(std::move(line_break));
    }
    lines_.push_back(std::move(line));
    breaks_.emplace_back();
    if (new_break_.empty())
    {
        new_break_ = DEFAULT_LINE_BREAK;
    }
}

std::string TextArea::Text() const
{
    std::string text;
    for (size_t i = 0; i < lines_.size(); ++i)
    {
        text += lines_[i];
        text += breaks_[i];
    }
    return text;
}

void TextArea::Clear()
{
    for (std::string &line : lines_)
    {
        ZeroMemory(line.data(), line.size());
    }
    lines_.assign(1, std::string{});
    breaks_.assign(1, std::string{});
    new_break_ = DEFAULT_LINE_BREAK;
    line_ = pos_ = 0;
    top_ = 0;
}

void TextArea::Insert(std::string_view text)
{
    lines_[line_].insert(pos_, text);
    pos_ += text.size();
}

void TextArea::NewLine()
{
    std::string &line = lines_[line_];
    std::string tail = line.substr(pos_);
    ZeroMemory(line.data() + pos_, line.size() - pos_);
    line.erase(pos_);
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(line_) + 1, std::move(tail));
    // The new line ends with the break this line had, this line with a new one
    breaks_.insert(breaks_.begin() + static_cast<std::ptrdiff_t>(line_), new_break_);
    ++line_;
    pos_ = 0;
}

void TextArea::Backspace()
{
    if (pos_ > 0)
    {
        const size_t prev = PrevPos(lines_[line_], pos_);
        lines_[line_].erase(prev, pos_ - prev);
        pos_ = prev;
    }
    else if (line_ > 0)
    {
        --line_;
        pos_ = lines_[line_].size();
        Delete();
    }
}

void TextArea::Delete()
{
    std::string &line = lines_[line_];
    if (pos_ < line.size())
    {
        line.erase(pos_, CharLen(line, pos_));
    }
    else if (line_ + 1 < lines_.size())
    {
        const auto next = static_cast<std::ptrdiff_t>(line_) + 1;
        line += lines_[line_ + 1];
        breaks_[line_] = breaks_[line_ + 1];
        ZeroMemory(lines_[line_ + 1].data(), lines_[line_ + 1].size());
        lines_.erase(lines_.begin() + next);
        breaks_.erase(breaks_.begin() + next);
    }
}

void TextArea::Left()
{
    if (pos_ > 0)
    {
        pos_ = PrevPos(lines_[line_], pos_);
    }
    else if (line_ > 0)
    {
        --line_;
        pos_ = lines_[line_].size();
    }
}

void TextArea::Right()
{
    if (pos_ < lines_[line_].size())
    {
        pos_ += CharLen(lines_[line_], pos_);
    }
    else if (line_ + 1 < lines_.size())
    {
        ++line_;
        pos_ = 0;
    }
}

void TextArea::Home()
{
    pos_ = 0;
}

void TextArea::End()
{
    pos_ = lines_[line_].size();
}

void TextArea::First()
{
    line_ = pos_ = 0;
}

void TextArea::Last()
{
    line_ = lines_.size() - 1;
    pos_ = lines_[line_].size();
}

std::vector<TextArea::Row> TextArea::Layout(int width) const
{
    width = std::max(1, width);
    std::vector<Row> rows;
    for (size_t i = 0; i < lines_.size(); ++i)
    {
        const std::string &line = lines_[i];
        size_t begin = 0;
        int row_width = 0;
        for (size_t p = 0; p < line.size();)
        {
            const size_t n = CharLen(line, p);
            const int w = CharWidth(line, p, n);
            // A character that doesn't fit starts the next row, even a wide one
            if (row_width + w > width && p > begin)
            {
                rows.push_back({i, begin, p});
                begin = p;
                row_width = 0;
            }
            row_width += w;
            p += n;
        }
        rows.push_back({i, begin, line.size()});
    }
    return rows;
}

size_t TextArea::CursorRow(const std::vector<Row> &rows) const
{
    for (size_t r = 0; r < rows.size(); ++r)
    {
        if (rows[r].line != line_)
            continue;
        // The end of a row is the start of the next one, except for the last row of a line
        const bool last_of_line = r + 1 == rows.size() || rows[r + 1].line != line_;
        if (pos_ < rows[r].end || last_of_line)
        {
            return r;
        }
    }
    return 0;
}

void TextArea::CursorPosition(int width, int &row, int &col) const
{
    const std::vector<Row> rows = Layout(width);
    const size_t r = CursorRow(rows);
    const std::string &line = lines_[line_];
    col = 0;
    for (size_t p = rows[r].begin; p < pos_;)
    {
        const size_t n = CharLen(line, p);
        col += CharWidth(line, p, n);
        p += n;
    }
    row = static_cast<int>(r);
}

void TextArea::MoveToColumn(const std::vector<Row> &rows, size_t r, int col)
{
    const Row &row = rows[r];
    const std::string &line = lines_[row.line];
    size_t p = row.begin;
    int w = 0;
    while (p < row.end)
    {
        const size_t n = CharLen(line, p);
        const int cw = CharWidth(line, p, n);
        if (w + cw > col)
            break;
        w += cw;
        p += n;
    }
    // The end of a row that isn't the last of its line is on the next row
    const bool last_of_line = r + 1 == rows.size() || rows[r + 1].line != row.line;
    if (p == row.end && !last_of_line && p > row.begin)
    {
        p = PrevPos(line, p);
    }
    line_ = row.line;
    pos_ = p;
}

bool TextArea::Up(int width)
{
    const std::vector<Row> rows = Layout(width);
    const size_t r = CursorRow(rows);
    if (r == 0)
    {
        return false;
    }
    int row, col;
    CursorPosition(width, row, col);
    MoveToColumn(rows, r - 1, col);
    return true;
}

bool TextArea::Down(int width)
{
    const std::vector<Row> rows = Layout(width);
    const size_t r = CursorRow(rows);
    if (r + 1 >= rows.size())
    {
        return false;
    }
    int row, col;
    CursorPosition(width, row, col);
    MoveToColumn(rows, r + 1, col);
    return true;
}

int TextArea::ScrollTop(int width, int height)
{
    height = std::max(1, height);
    const std::vector<Row> rows = Layout(width);
    const int r = static_cast<int>(CursorRow(rows));
    if (r < top_)
    {
        top_ = r;
    }
    if (r >= top_ + height)
    {
        top_ = r - height + 1;
    }
    top_ = std::max(0, std::min(top_, static_cast<int>(rows.size()) - height));
    return top_;
}

std::string TextArea::RowText(const Row &row) const
{
    const std::string &line = lines_.at(row.line);
    std::string text;
    for (size_t p = row.begin; p < row.end;)
    {
        const size_t n = CharLen(line, p);
        text += Shown(std::string_view(line).substr(p, n));
        p += n;
    }
    return text;
}
