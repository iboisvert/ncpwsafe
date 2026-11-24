/* Copyright 2026 Ian Boisvert */
#include "PasswordPrompt.h"

#ifdef _WIN32

#include "Win32.h"

PasswordPromptResult PromptForPassword(const char *prompt, std::string &password)
{
    return win32::ReadConsolePassword(prompt, password);
}

#else

#include "Utils.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace
{

/**
 * The signals caught while the prompt is shown, so that the terminal's settings
 * are restored before they take their usual action
 */
constexpr int CAUGHT_SIGNALS[] = {SIGHUP, SIGINT, SIGQUIT, SIGTERM, SIGTSTP, SIGTTIN, SIGTTOU};
constexpr size_t CAUGHT_SIGNAL_COUNT = sizeof(CAUGHT_SIGNALS) / sizeof(CAUGHT_SIGNALS[0]);

volatile std::sig_atomic_t caught_signal = 0;

void OnSignal(int sig)
{
    caught_signal = sig;
}

/**
 * Catch `CAUGHT_SIGNALS` for the lifetime of the object, except those that are ignored.
 * Without `SA_RESTART`, so that a signal interrupts `read()`.
 */
class ScopedSignalCatcher
{
    struct sigaction saved_[CAUGHT_SIGNAL_COUNT];
    bool installed_[CAUGHT_SIGNAL_COUNT] = {};

public:
    ScopedSignalCatcher()
    {
        caught_signal = 0;
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = OnSignal;
        sigemptyset(&sa.sa_mask);
        for (size_t i = 0; i < CAUGHT_SIGNAL_COUNT; ++i)
        {
            struct sigaction &saved = saved_[i];
            if (sigaction(CAUGHT_SIGNALS[i], nullptr, &saved) != 0)
                continue;
            // A signal that is ignored (e.g. SIGHUP under nohup) stays ignored
            if (!(saved.sa_flags & SA_SIGINFO) && saved.sa_handler == SIG_IGN)
                continue;
            installed_[i] = sigaction(CAUGHT_SIGNALS[i], &sa, nullptr) == 0;
        }
    }

    ~ScopedSignalCatcher()
    {
        for (size_t i = 0; i < CAUGHT_SIGNAL_COUNT; ++i)
        {
            if (installed_[i])
                sigaction(CAUGHT_SIGNALS[i], &saved_[i], nullptr);
        }
    }

    ScopedSignalCatcher(const ScopedSignalCatcher &) = delete;
    ScopedSignalCatcher &operator=(const ScopedSignalCatcher &) = delete;
};

/** Append `c` to `str`. When `str` has to grow, its old buffer is zeroed rather than left on the heap */
void AppendSecret(std::string &str, char c)
{
    if (str.size() == str.capacity())
    {
        std::string larger;
        larger.reserve(str.capacity() * 2);
        larger.append(str);
        ZeroString(str);
        str.swap(larger);
    }
    str.push_back(c);
}

void WriteAll(int fd, const char *text)
{
    size_t len = std::strlen(text);
    while (len > 0)
    {
        ssize_t n = write(fd, text, len);
        if (n < 0)
        {
            if (errno == EINTR && !caught_signal)
                continue;
            return;
        }
        text += n;
        len -= static_cast<size_t>(n);
    }
}

/**
 * Read a line from `fd` and append it to `line`, without its line ending.
 * @returns 1 for a line, 0 if input ended before a line ending,
 * -1 on an error or a caught signal, with `errno` set
 */
int ReadLine(int fd, std::string &line)
{
    int status = -1;
    char c = 0;
    for (;;)
    {
        if (caught_signal)
        {
            errno = EINTR;
            break;
        }
        ssize_t n = read(fd, &c, 1);
        if (n < 0)
        {
            if (errno == EINTR && !caught_signal)
                continue;
            break;
        }
        if (n == 0)
        {
            status = 0;
            break;
        }
        if (c == '\n' || c == '\r')
        {
            status = 1;
            break;
        }
        AppendSecret(line, c);
    }
    ZeroMemory(&c, sizeof(c));
    return status;
}

}  // namespace

PasswordPromptResult PromptForPassword(const char *prompt, std::string &password)
{
    // The terminal's settings before the first prompt. When the process is stopped
    // while the settings can't be restored (moved to the background), the prompt
    // starts again, and these are the settings to go back to
    struct termios original;
    bool have_original = false;

    for (;;)
    {
        int fd = open("/dev/tty", O_RDWR | O_NOCTTY | O_CLOEXEC);
        if (fd < 0)
        {
            // ENXIO: the process has no controlling terminal
            return PasswordPromptResult::NO_TERMINAL;
        }
        struct termios current;
        if (tcgetattr(fd, &current) != 0)
        {
            int saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return PasswordPromptResult::NO_TERMINAL;
        }
        if (!have_original)
        {
            original = current;
            have_original = true;
        }

        std::string line;
        // Most passwords fit without growing, see AppendSecret()
        line.reserve(256);
        int status = -1;
        int saved_errno = 0;
        int sig = 0;
        {
            ScopedSignalCatcher catcher;

            struct termios noecho = original;
            noecho.c_lflag &= ~(ECHO | ECHONL);
            // Line editing (erase, kill) still works
            noecho.c_lflag |= ICANON;
            // Echo is turned off before the prompt is shown, and what was typed before it is discarded
            if (tcsetattr(fd, TCSAFLUSH, &noecho) == 0)
            {
                WriteAll(fd, prompt);
                status = ReadLine(fd, line);
                saved_errno = errno;
                // Enter wasn't echoed either
                WriteAll(fd, "\n");
                // Also discards anything typed after Enter, which wasn't echoed
                while (tcsetattr(fd, TCSAFLUSH, &original) != 0 && errno == EINTR
                       && caught_signal != SIGTTOU)
                {
                    // Interrupted, try again
                }
            }
            else
            {
                // Never read a password that would be echoed
                saved_errno = errno;
            }
            sig = caught_signal;
        }  // The signal handlers are restored
        close(fd);

        if (sig != 0)
        {
            ZeroString(line);
            // The signal's usual action, now that the terminal is restored: most end the process
            raise(sig);
            if (sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU)
            {
                // Stopped, then continued: prompt again
                continue;
            }
            errno = EINTR;
            return PasswordPromptResult::FAILED;
        }

        if (status == 1)
        {
            password.swap(line);
            // Now the previous value of `password`
            ZeroString(line);
            return PasswordPromptResult::OK;
        }
        ZeroString(line);
        if (status == 0)
        {
            return PasswordPromptResult::CANCELLED;
        }
        errno = saved_errno;
        return PasswordPromptResult::FAILED;
    }
}

#endif  // #ifdef _WIN32
