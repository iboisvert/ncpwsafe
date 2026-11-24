/* Copyright 2026 Ian Boisvert */
#ifndef HAVE_PASSWORDPROMPT_H
#define HAVE_PASSWORDPROMPT_H

#include <string>

/** Result of `PromptForPassword()` */
enum class PasswordPromptResult
{
    OK,
    NO_TERMINAL,  ///< The process has no terminal (console on Windows) to prompt on
    CANCELLED,    ///< Input ended (Ctrl-D) before a line was entered
    TOO_LONG,     ///< The line reached the terminal's limit, so part of it may have been dropped
    FAILED,       ///< Reading from or setting up the terminal failed, with `errno` set
};

/**
 * Prompt for a password on the terminal, and read it without echoing it.
 *
 * The prompt is written to and the password read from the controlling terminal,
 * `/dev/tty` (on Windows, the console, `CONIN$` and `CONOUT$`), not standard input
 * and output, so a password is never read from a pipe or file by mistake. Input
 * typed before the prompt is shown is discarded.
 *
 * The password is the line typed, without its line ending. Enter ends the line
 * also on a terminal set to `-icrnl`. A line as long as the terminal keeps (4095
 * bytes on Linux) is refused rather than taken cut short. Memory that held the
 * password, other than `password`, is zeroed.
 *
 * The terminal's settings are restored before returning, and also when a signal
 * that ends the process (e.g. Ctrl-C) arrives while the prompt is shown: the
 * settings are restored, then the signal takes its usual action; when several
 * arrive, one that ends the process wins over one that stops it. When the process
 * is stopped (Ctrl-Z) and continued, the prompt starts again.
 *
 * @param prompt The text to show, e.g. `"Password: "`
 * @param[out] password The password typed, set only if `OK` is returned
 */
PasswordPromptResult PromptForPassword(const char *prompt, std::string &password);

#endif  // #ifndef HAVE_PASSWORDPROMPT_H
