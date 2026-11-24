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
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace
{

/**
 * The signals caught while the prompt is shown, so that the terminal's settings
 * are restored before they take their usual action. The signals that end the
 * process come first: when several arrive, the first of them in this list is the
 * one that is raised again (see `CaughtSignal()`).
 */
constexpr int CAUGHT_SIGNALS[] = {SIGHUP, SIGINT, SIGQUIT, SIGTERM, SIGTSTP, SIGTTIN, SIGTTOU};
constexpr size_t CAUGHT_SIGNAL_COUNT = sizeof(CAUGHT_SIGNALS) / sizeof(CAUGHT_SIGNALS[0]);

/** For each signal of `CAUGHT_SIGNALS`, nonzero once it was caught */
volatile std::sig_atomic_t caught[CAUGHT_SIGNAL_COUNT];

void OnSignal(int sig)
{
    for (size_t i = 0; i < CAUGHT_SIGNAL_COUNT; ++i)
    {
        if (CAUGHT_SIGNALS[i] == sig)
            caught[i] = 1;
    }
}

/** The first of `CAUGHT_SIGNALS` that was caught, or 0 */
int CaughtSignal()
{
    for (size_t i = 0; i < CAUGHT_SIGNAL_COUNT; ++i)
    {
        if (caught[i])
            return CAUGHT_SIGNALS[i];
    }
    return 0;
}

bool IsStopSignal(int sig)
{
    return sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU;
}

/**
 * Catch `CAUGHT_SIGNALS` for the lifetime of the object, except those that are ignored.
 * Without `SA_RESTART`, so that a signal interrupts `tcsetattr()` and `pselect()`.
 */
class ScopedSignalCatcher
{
    struct sigaction saved_[CAUGHT_SIGNAL_COUNT];
    bool installed_[CAUGHT_SIGNAL_COUNT] = {};

public:
    ScopedSignalCatcher()
    {
        for (size_t i = 0; i < CAUGHT_SIGNAL_COUNT; ++i)
            caught[i] = 0;
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

/**
 * Block `CAUGHT_SIGNALS` for the lifetime of the object. A signal that arrives
 * meanwhile is delivered by `pselect()` with `Unblocked()`, or when the object is
 * destroyed. So a signal can't arrive between checking for one and starting to
 * wait for input, where it wouldn't interrupt the wait.
 */
class ScopedSignalBlock
{
    sigset_t saved_;

public:
    ScopedSignalBlock()
    {
        sigset_t block;
        sigemptyset(&block);
        for (int sig : CAUGHT_SIGNALS)
            sigaddset(&block, sig);
        sigprocmask(SIG_BLOCK, &block, &saved_);
    }

    ~ScopedSignalBlock()
    {
        sigprocmask(SIG_SETMASK, &saved_, nullptr);
    }

    /** The signal mask before the object blocked the signals */
    const sigset_t &Unblocked() const
    {
        return saved_;
    }

    ScopedSignalBlock(const ScopedSignalBlock &) = delete;
    ScopedSignalBlock &operator=(const ScopedSignalBlock &) = delete;
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
            if (errno == EINTR)
                continue;
            return;
        }
        text += n;
        len -= static_cast<size_t>(n);
    }
}

/**
 * Read a line from `fd` and append it to `line`, without its line ending.
 * Call with `CAUGHT_SIGNALS` blocked: they are unblocked, as `wait_mask`, only while
 * waiting for input, so one that arrives always ends the wait.
 * @returns 1 for a line, 0 if input ended before a line ending,
 * -1 on an error or a caught signal, with `errno` set
 */
int ReadLine(int fd, std::string &line, const sigset_t &wait_mask)
{
    int status = -1;
    char c = 0;
    for (;;)
    {
        if (CaughtSignal())
        {
            errno = EINTR;
            break;
        }
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        if (pselect(fd + 1, &fds, nullptr, nullptr, nullptr, &wait_mask) < 0)
        {
            // EINTR: a caught signal is found above, any other is ignored
            if (errno == EINTR)
                continue;
            break;
        }
        ssize_t n = read(fd, &c, 1);
        if (n < 0)
        {
            if (errno == EINTR)
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

/**
 * The length from which the terminal may have dropped what was typed on a line.
 * In canonical mode, Linux keeps 4095 bytes of a line (`N_TTY_BUF_SIZE` less one)
 * and discards what is typed after them, except the line ending.
 */
size_t LineLimit([[maybe_unused]] int fd)
{
#ifdef __linux__
    return 4095;
#else
    long max_canon = fpathconf(fd, _PC_MAX_CANON);
    return max_canon > 1 ? static_cast<size_t>(max_canon) - 1 : SIZE_MAX;
#endif
}

}  // namespace

PasswordPromptResult PromptForPassword(const char *prompt, std::string &password)
{
    // The terminal's settings before the first prompt, which the prompt goes back to,
    // also when it starts again after the process is stopped
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
            // Enter ends the line, also on a terminal set to -icrnl
            noecho.c_iflag |= ICRNL;
            noecho.c_iflag &= ~IGNCR;
            // Echo is turned off before the prompt is shown, and what was typed before it
            // is discarded. With the signals not blocked yet, a process in the background
            // gets SIGTTOU here, and stops
            bool echo_off = tcsetattr(fd, TCSAFLUSH, &noecho) == 0;
            saved_errno = errno;
            {
                ScopedSignalBlock blocked;
                // Never read a password that would be echoed
                if (echo_off && !CaughtSignal())
                {
                    WriteAll(fd, prompt);
                    status = ReadLine(fd, line, blocked.Unblocked());
                    saved_errno = errno;
                    // Enter wasn't echoed either
                    WriteAll(fd, "\n");
                }
                if (echo_off)
                {
                    // Also discards anything typed after Enter, which wasn't echoed. With
                    // SIGTTOU blocked, this works also after being moved to the background
                    tcsetattr(fd, TCSAFLUSH, &original);
                }
            }  // A signal that arrived meanwhile is caught now
            sig = CaughtSignal();
        }  // The signal handlers are restored
        const size_t line_limit = LineLimit(fd);
        close(fd);

        if (sig != 0)
        {
            ZeroString(line);
            // The signal's usual action, now that the terminal is restored: most end the process
            raise(sig);
            if (IsStopSignal(sig))
            {
                // Stopped, then continued: prompt again
                continue;
            }
            errno = EINTR;
            return PasswordPromptResult::FAILED;
        }

        if (status == 1 && line.size() >= line_limit)
        {
            // Part of what was typed may be missing
            ZeroString(line);
            return PasswordPromptResult::TOO_LONG;
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
