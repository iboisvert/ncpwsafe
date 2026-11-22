/* Copyright 2026 Ian Boisvert */
#ifndef HAVE_TEXTAREA_H
#define HAVE_TEXTAREA_H

#include <string>
#include <string_view>
#include <vector>

/**
 * The text of a multi-line dialog field, such as the notes of an account, and a
 * cursor in it.
 *
 * A form field can't hold this text as it is: ncurses drops what comes after a line
 * break, after a carriage return or after a wide character on the first row, and turns
 * tabs into spaces. So `Dialog` draws a `TextArea` over an empty field and gives it the
 * keys. The text is kept as lines with the line break that ended each one, so the text
 * that isn't edited stays exactly as it was stored, whatever line breaks it uses.
 *
 * Rows are laid out for a width in columns (see `DisplayWidth()`), a line longer than
 * the width takes more than one row. Positions are byte offsets in a line, always at the
 * start of a character. There is no ncurses code here.
 */
class TextArea
{
public:
    /** A row of the layout: the bytes [`begin`, `end`) of line `line` */
    struct Row
    {
        size_t line;
        size_t begin;
        size_t end;
    };

    /** The line break that `NewLine()` uses when the text has none, as Password Safe does */
    static constexpr const char *DEFAULT_LINE_BREAK = "\r\n";

    explicit TextArea(std::string_view text = {});
    ~TextArea();
    TextArea(const TextArea &) = delete;
    TextArea &operator=(const TextArea &) = delete;
    TextArea(TextArea &&) = default;
    TextArea &operator=(TextArea &&) = default;

    /** Replace the text, the cursor goes to the start */
    void SetText(std::string_view text);
    /** The text, with the line breaks it was given and the ones `NewLine()` added */
    std::string Text() const;
    /** Zero the text in memory and empty it */
    void Clear();

    size_t LineCount() const
    {
        return lines_.size();
    }
    const std::string &Line(size_t i) const
    {
        return lines_.at(i);
    }
    /** The cursor: a line and a byte offset in it */
    size_t CursorLine() const
    {
        return line_;
    }
    size_t CursorPos() const
    {
        return pos_;
    }

    /** Insert text (no line breaks) at the cursor, the cursor moves past it */
    void Insert(std::string_view text);
    /** Break the line at the cursor */
    void NewLine();
    /** Delete the character before the cursor, or join the line with the one before */
    void Backspace();
    /** Delete the character at the cursor, or join the line with the one after */
    void Delete();

    void Left();
    void Right();
    /** The start of the cursor's line */
    void Home();
    /** The end of the cursor's line */
    void End();
    /** The start of the text */
    void First();
    /** The end of the text */
    void Last();
    /**
     * Move to the row above, at the column nearest the cursor's.
     * @returns `false` if the cursor is on the first row, it doesn't move
     */
    bool Up(int width);
    /** As `Up()`, to the row below; `false` if the cursor is on the last row */
    bool Down(int width);

    /** The rows of the text for `width` columns; an empty line takes one row */
    std::vector<Row> Layout(int width) const;
    /** The row of the cursor in `Layout(width)`, and its column */
    void CursorPosition(int width, int &row, int &col) const;
    /**
     * The first row to show in `height` rows of `width` columns: the last one,
     * moved just enough to show the cursor
     */
    int ScrollTop(int width, int height);
    /**
     * The text of `row` as it is shown: tabs as spaces, and characters that can't be
     * shown as '?'. It takes as many columns as the row in `Layout()`.
     */
    std::string RowText(const Row &row) const;

private:
    /** The row in `rows` that the cursor is on */
    size_t CursorRow(const std::vector<Row> &rows) const;
    /** Put the cursor in `row` at the column nearest `col` */
    void MoveToColumn(const std::vector<Row> &rows, size_t row, int col);

    std::vector<std::string> lines_;
    /** The line break that ends each line, empty for the last one */
    std::vector<std::string> breaks_;
    /** The line break for `NewLine()`: the first one in the text, or `DEFAULT_LINE_BREAK` */
    std::string new_break_;
    size_t line_ = 0;
    size_t pos_ = 0;
    int top_ = 0;
};

#endif  // #ifndef HAVE_TEXTAREA_H
