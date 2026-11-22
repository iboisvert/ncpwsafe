/* Copyright 2020 Ian Boisvert */
#include "PWSafeApp.h"
#include "ExportDbCommand.h"
#include "ChangeDbPasswordCommand.h"
#include "GeneratePasswordCommand.h"
#include "GeneratePasswordDlg.h"
#include "GenerateTestDbCommand.h"
#include "ProgArgs.h"
#include "Utils.h"

#include "libpwsafe.h"
#ifdef HAVE_GLOG
#include "libglog.h"
#endif

#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <getopt.h>
#include <wchar.h>
#ifdef _WIN32
#include <filesystem>
#include "Win32.h"
#else
#include <libgen.h>
#endif
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

using std::fprintf;
using std::srand;

/** Get the program name to display from `argv[0]` */
static std::string ProgramName(char *argv0)
{
#ifdef _WIN32
    // Strip directory and `.exe` extension
    return std::filesystem::path(argv0).stem().string();
#else
    return basename(argv0);
#endif
}

/**
 * Prevent ptrace and the creation of core dumps (on Windows, opening the process
 * and crash dumps), which could expose decrypted passwords.
 * Only in Release builds, so that a debugger can attach to Debug builds.
 */
static bool DisableCoreDump()
{
#if !defined(NDEBUG)
    return true;
#elif defined(_WIN32)
    win32::DisableCrashDumps();
    if (!win32::RestrictProcessAccess())
    {
        errno = EPERM;  // For the error message
        return false;
    }
    return true;
#elif defined(HAVE_SYS_PRCTL_H)
    return prctl(PR_SET_DUMPABLE, 0) == 0;
#else
    return true;
#endif
}

static void Usage(const char *progName)
{
    using namespace std;

    // clang-format off
    fprintf(stdout,
            "Usage: %s [COMMAND] [OPTIONS] [DATABASE_FILE]\n"
            "where COMMAND is one of:\n"
            "  <default>            Open an account database\n"
#ifndef NDEBUG
            "  --test-db            Generate a test account database\n"
#endif
            "  --generate-password  Generate a list of passwords,\n"
            "                       does not change the account database\n"
            "  --export-db          Export account database as plain text\n"
            "  --change-password    Change the account databasse password\n"
            "\n"
            "Common options:\n"
            "  -c,--config=PATHNAME Specify the configuration file\n"
            "                       Default is %s\n"
            "  DATABASE_FILE        The account database file\n"
            "  -P,--password=STR    Account database password. Deprecated, the password can\n"
            "                       be seen by other users, use --password-file instead\n"
            "  --password-file=PATHNAME\n"
            "                       Read the account database password from the first\n"
            "                       line of a file\n"
            "  --force              Force overwrite output file\n"
            "\n"
            "Open database options:\n"
            "  -r,--read-only       Open database as read-only\n"
            "\n"
            "Change account database options:\n"
            "  --new-password=STR   New account database password. Deprecated, use\n"
            "                       --new-password-file instead\n"
            "  --new-password-file=PATHNAME\n"
            "                       Read the new account database password from the first\n"
            "                       line of a file\n"
            "\n"
            "Generate test database options:\n"
            "  --test-db-lang=STR   Language of strings in generated database\n"
            "                       Valid values are en-CA, ru. Default value is %s\n"
            "  --test-db-groups=N   Number of groups in the generated database\n"
            "                       Default value is %lu\n"
            "  --test-db-items=N    Number of itesm in the generated database\n"
            "                       Default value is %lu\n"
            "\n"
            "Generate password options:\n"
            "  --password-count=N   Generate N passwords, default count is %zd\n"
            "  --password-length=N  Length of generated passwords\n"
            "                       Default length is %zd\n"
            "  --password-policy=CHOICE\n"
            "                       where CHOICE is an index or string of value:\n"
            "                       1=%s\n"
            "                       2=%s\n"
            "                       3=%s\n"
            "                       4=%s\n"
            "                       5=%s\n"
            "                       Default CHOICE=%s\n"
            "\n"
            "Export account database options:\n"
            "  -o,--out=PATHNAME   Output file\n"
            "\n"
            "Help options:\n"
            "  -h,--help            Display this help text and exit\n",
            progName, 
            DEFAULT_CONFIG_FILE.c_str(),
            DEFAULT_GENERATE_LANGUAGE, DEFAULT_GENERATE_GROUP_COUNT, DEFAULT_GENERATE_ITEM_COUNT,
            DEFAULT_GENERATE_PASSWORD_COUNT,
            DEFAULT_PASSWORD_LENGTH,
            PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(0)), PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(1)), PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(2)), PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(3)), PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(4)),
            PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(DEFAULT_PASSWORD_POLICY-1))
    );
    // clang-format on
}

static bool Error(const char *fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    return false;
}

static bool ValidateArgs(InputProgArgs &args)
{
    bool result = true;

    size_t ncmds = (args.cmd_generate_password_ ? 1 : 0)
            + (args.cmd_export_db_ ? 1 : 0) 
            + (args.cmd_generate_test_db_ ? 1 : 0) 
            + (args.cmd_change_db_password_ ? 1 : 0);
    if (ncmds > 1)
    {
        result = Error("Only one command may be specified\n");
    }

#ifdef HAVE_GLOG
    LOG_IF(INFO, args.config_file_ && !fs::Exists(*args.config_file_)) 
        << "Configuration file \"" << *args.config_file_ << "\" does not exist";
#endif

    Operation cmd = args.GetCommand();
    if (args.database_)
    {
        if (cmd != Operation::OPEN_DB 
            && cmd != Operation::GENERATE_TEST_DB 
            && cmd != Operation::EXPORT_DB
            && cmd != Operation::CHANGE_DB_PASSWORD)
            result = false;
    }
    if (args.password_)
    {
        if (cmd != Operation::OPEN_DB 
            && cmd != Operation::GENERATE_TEST_DB 
            && cmd != Operation::EXPORT_DB
            && cmd != Operation::CHANGE_DB_PASSWORD)
            result = false;
    }
    if (args.read_only_)
    {
        if (cmd != Operation::OPEN_DB && cmd != Operation::GENERATE_TEST_DB)
            result = false;
    }
    if (args.generate_language_)
    {
        if (cmd != Operation::GENERATE_TEST_DB)
            result = false;
        else
        {
            if (args.generate_language_->compare("en-CA") != 0 && args.generate_language_->compare("ru") != 0)
                result = Error("Valid languages for test database are: en-CA, ru\n");
        }
    }
    if (args.generate_group_count_)
    {
        if (cmd != Operation::GENERATE_TEST_DB)
            result = false;
        else
        {
            if (*args.generate_group_count_ < 1)
            {
                result = Error("Test account database group count must be > 0\n");
            }
        }
    }
    if (args.generate_item_count_)
    {
        if (cmd != Operation::GENERATE_TEST_DB)
            result = false;
        else
        {
            if (*args.generate_item_count_ < 1)
            {
                result = Error("Test account database item count must be > 0\n");
            }
        }
    }
    if (args.generate_password_count_)
    {
        if (cmd != Operation::GENERATE_PASSWORD)
            result = false;
        else
        {
            if (*args.generate_password_count_ < 1)
            {
                result = Error("Generated password count must be > 0\n");
            }
        }
    }
    if (args.password_policy_)
    {
        if (cmd != Operation::GENERATE_PASSWORD)
            result = false;
        else
        {
            if (*args.password_policy_ < 1 || *args.password_policy_ > PasswordPolicy::Composition::COUNT)
            {
                result = Error("Invalid policy for generated passwords\n");
            }
        }
    }
    if (args.password_length_)
    {
        if (cmd != Operation::GENERATE_PASSWORD)
            result = false;
        else
        {
            if (*args.password_length_ < 1)
            {
                result = Error("Generated password length must be > 0\n");
            }
        }
    }

    if (cmd == Operation::GENERATE_TEST_DB)
    {
        if (!args.database_ || args.database_->empty())
        {
            result = Error("Account database file is required\n");
        }
        else if (!args.force_ && fs::Exists(*args.database_))
        {
            result = Error("File %s already exists\n", args.database_->c_str());
        }
        if (!args.password_ || args.password_->empty())
        {
            result = Error("Account database password is required\n");
        }
    }

    if (cmd == Operation::EXPORT_DB)
    {
        if (!args.database_ || args.database_->empty())
        {
            result = Error("Account database file is required\n");
        }
        else if (!fs::Exists(*args.database_))
        {
            result = Error("Account database file %s does not exist\n", args.database_->c_str());
        }
        if (!args.password_ || args.password_->empty())
        {
            result = Error("Account database password is required\n");
        }
        if (!args.output_file_ || args.output_file_->empty())
        {
            result = Error("Output file is required\n");
        }
        else if (!args.force_ && fs::Exists(*args.output_file_))
        {
            result = Error("Output file %s already exists\n", args.output_file_->c_str());
        }
    }

    if (cmd == Operation::CHANGE_DB_PASSWORD)
    {
        if (!args.database_ || args.database_->empty())
        {
            result = Error("Account database file is required\n");
        }
        else if (!fs::Exists(*args.database_))
        {
            result = Error("Account database file %s does not exist\n", args.database_->c_str());
        }
        if (!args.password_ || args.password_->empty())
        {
            result = Error("Account database password is required\n");
        }
        if (!args.new_password_ || args.new_password_->empty())
        {
            result = Error("New account database password is required\n");
        }
    }
    return result;
}

enum
{
    O_GENERATE_TEST_DB = 1024,
    O_TEST_DB_LANG,
    O_TEST_DB_GROUPS,
    O_TEST_DB_ITEMS,
    O_GENERATE_PASSWORD,
    O_PASSWORD_COUNT,
    O_PASSWORD_POLICY,
    O_PASSWORD_LENGTH,
    O_EXPORT_DB,
    O_CHANGE_PASSWORD,
    O_NEW_PASSWORD,
    O_PASSWORD_FILE,
    O_NEW_PASSWORD_FILE,
    OPT_FORCE
};

// clang-format off
static constexpr struct option options[] {
    {"test-db", no_argument, nullptr, O_GENERATE_TEST_DB}, 
    {"test-db-lang", required_argument, nullptr, O_TEST_DB_LANG},
    {"test-db-groups", required_argument, nullptr, O_TEST_DB_GROUPS},
    {"test-db-items", required_argument, nullptr, O_TEST_DB_ITEMS},
    {"help", no_argument, nullptr, 'h'},
    {"password", required_argument, nullptr, 'P'}, 
    {"password-file", required_argument, nullptr, O_PASSWORD_FILE},
    {"read-only", no_argument, nullptr, 'r'},
    {"generate-password", no_argument, nullptr, O_GENERATE_PASSWORD},
    {"password-count", required_argument, nullptr, O_PASSWORD_COUNT},
    {"password-policy", required_argument, nullptr, O_PASSWORD_POLICY},
    {"password-length", required_argument, nullptr, O_PASSWORD_LENGTH},
    {"export-db", no_argument, nullptr, O_EXPORT_DB},
    {"out", required_argument, nullptr, 'o'},
    {"change-password", no_argument, nullptr, O_CHANGE_PASSWORD},
    {"new-password", required_argument, nullptr, O_NEW_PASSWORD},
    {"new-password-file", required_argument, nullptr, O_NEW_PASSWORD_FILE},
    {"force", no_argument, nullptr, OPT_FORCE},
    {"config", required_argument, nullptr, 'c'},
    {nullptr, 0, nullptr, 0},
};
// clang-format on

/**
 * Overwrite a command-line argument in place, so that it is no longer visible
 * in /proc/<pid>/cmdline (e.g. to ps). It is visible until then.
 */
static void HideArg(char *arg)
{
    ZeroMemory(arg, strlen(arg));
}

/**
 * If `pathname` is set, read `password` from the file it names.
 * @param option The name of the option that gives the password directly
 * @returns `false` if both the option and the file are given, or the file can't be read
 */
static bool ReadPasswordFileArg(const char *option, const std::optional<std::string> &pathname,
    std::optional<std::string> &password)
{
    if (!pathname)
    {
        return true;
    }
    if (password)
    {
        return Error("Only one of %s and %s-file may be specified\n", option, option);
    }
    std::string value;
    if (!ReadPasswordFile(*pathname, value))
    {
        return Error("Unable to read password file %s: %s\n", pathname->c_str(), strerror(errno));
    }
    password = std::move(value);
    return true;
}

static bool ParseArgs(int argc, char *const argv[], InputProgArgs &args)
{
    args.prog_name_ = argv[0];

    if (argc < 2)
        return true;

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "c:o:P:r", options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'c': {
            assert(optarg);
            args.config_file_ = optarg;
            break;
        }
        case OPT_FORCE: {
            args.force_ = true;
            break;
        }
        case O_GENERATE_PASSWORD: {
            args.cmd_generate_password_ = true;
            break;
        }
        case O_PASSWORD_COUNT: {
            assert(optarg);
            char *pstr = optarg, *pend;
            args.generate_password_count_ = strtoul(pstr, &pend, 10);
            break;
        }
        case O_PASSWORD_POLICY: {
            assert(optarg);
            // A 1-based policy number, or a policy name. Anything else is rejected by ValidateArgs()
            char *pstr = optarg, *pend;
            size_t policy = strtoul(pstr, &pend, 10);
            if (pend == pstr || *pend != '\0')
            {
                policy = 0;
                for (size_t i = 0; i < PasswordPolicy::Composition::COUNT; ++i)
                {
                    const char *policyName = PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(i));
                    if (strcmp(policyName, optarg) == 0)
                    {
                        policy = i + 1;
                        break;
                    }
                }
            }
            args.password_policy_ = policy;
            break;
        }
        case O_PASSWORD_LENGTH: {
            assert(optarg);
            args.password_length_ = strtoul(optarg, nullptr, 10);
            break;
        }
        case O_EXPORT_DB: {
            args.cmd_export_db_ = true;
            break;
        }
        case 'o': {
            assert(optarg);
            args.output_file_ = optarg;
            break;
        }
        case O_CHANGE_PASSWORD: {
            args.cmd_change_db_password_ = true;
            break;
        }
        case O_NEW_PASSWORD: {
            assert(optarg);
            args.new_password_ = optarg;
            HideArg(optarg);
            fprintf(stderr, "Warning: --new-password is deprecated, the password can be seen by other users. "
                "Use --new-password-file instead.\n");
            break;
        }
        case O_NEW_PASSWORD_FILE: {
            assert(optarg);
            args.new_password_file_ = optarg;
            break;
        }
        case O_GENERATE_TEST_DB: {
            args.cmd_generate_test_db_ = true;
            break;
        }
        case O_TEST_DB_LANG: {
            assert(optarg);
            args.generate_language_ = optarg;
            break;
        }
        case O_TEST_DB_GROUPS: {
            assert(optarg);
            args.generate_group_count_ = strtoul(optarg, nullptr, 10);
            break;
        }
        case O_TEST_DB_ITEMS: {
            assert(optarg);
            args.generate_item_count_ = strtoul(optarg, nullptr, 10);
            break;
        }
        case 'P': {
            assert(optarg);
            args.password_ = optarg;
            HideArg(optarg);
            fprintf(stderr, "Warning: --password is deprecated, the password can be seen by other users. "
                "Use --password-file instead.\n");
            break;
        }
        case O_PASSWORD_FILE: {
            assert(optarg);
            args.password_file_ = optarg;
            break;
        }
        case 'r': {
            args.read_only_ = true;
            break;
        }
        case 'h': {
            return false;
        }
        case '?': {
            // Unknown argument
            return false;
        }
        }
    }

    int count = argc - optind;
    if (count == 1)
    {
        const char *filename = argv[optind];
        args.database_ = filename;
    }
    else if (count > 1)
    {
        return false;
    }

    if (!ReadPasswordFileArg("--password", args.password_file_, args.password_)
        || !ReadPasswordFileArg("--new-password", args.new_password_file_, args.new_password_))
    {
        return false;
    }

    bool status = ValidateArgs(args);
    return status;
}

int main(int argc, char *argv[])
{
#ifdef HAVE_GLOG
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);
#endif

#ifdef _WIN32
    // UTF-8 console code pages and locale
    win32::InitConsole();
#else
    // ncurses needs a UTF-8 character set to show and edit non-ASCII account text.
    // Only LC_CTYPE, so that e.g. number formats don't change
    SetUtf8CtypeLocale();
#endif

    // Don't allow ptrace or core dumps in Release builds
    if (!DisableCoreDump())
    {
        fprintf(stderr, "Unable to disable core dumps: %s\n", strerror(errno));
        return PRC_ERR_FAIL;
    }

    InputProgArgs input_args;
    if (!ParseArgs(argc, argv, input_args))
    {
        Usage(ProgramName(argv[0]).c_str());
        return PRC_ERR_FAIL;
    }

    srand(time(0));

    PWSafeApp app;
    app.Init(input_args);
    // The passwords have been copied to the app, wipe these copies
    if (input_args.password_) ZeroString(*input_args.password_);
    if (input_args.new_password_) ZeroString(*input_args.new_password_);
    const ProgArgs &args = app.GetArgs();

    int result = static_cast<int>(RC_SUCCESS);
    switch (args.command_)
    {
    case Operation::OPEN_DB: 
    {
        DialogResult rc = app.Show();
        result = static_cast<int>(rc == DialogResult::OK ? RC_SUCCESS : RC_FAILURE);
        app.SavePrefs();
        if (app.SignedOff())
        {
            fprintf(stderr, "%s: signed off after %s without input\n", NCPWSAFE_APPNAME,
                    FormatDuration(app.AutoSignOffSeconds()).c_str());
            if (app.SignOffDiscardedChanges())
            {
                fprintf(stderr, "Unsaved changes were discarded.\n");
            }
            result = EXIT_AUTO_SIGN_OFF;
        }
        break;
    }
    case Operation::CHANGE_DB_PASSWORD: 
    {
        int rc = ChangeDbPasswordCommand{app, args.new_password_}.Execute();
        if (rc == RC_SUCCESS)
        {
            fprintf(stdout, "Account database password changed\n");
        }
        else
        {
            result = static_cast<int>(RC_FAILURE);
            if (rc == RC_ERR_INCORRECT_PASSWORD)
            {
                fprintf(stderr, "Incorrect account database password\n");
            }
            else
            {
                fprintf(stderr, "Changing account database password failed\n");
            }
        }
        fflush(stdout);
        break;
    }
    case Operation::GENERATE_TEST_DB:
    {
        fprintf(stdout, "Generating test database at %s\n", args.database_.c_str());
        [[maybe_unused]] int rc = GenerateTestDbCommand{app, args.force_,
            args.generate_language_, args.generate_group_count_, args.generate_item_count_}.Execute();
        fflush(stdout);
        break;
    }
    case Operation::GENERATE_PASSWORD: 
    {
        size_t count = args.generate_password_count_;
        size_t length = args.password_length_;
        int policyIndex = args.password_policy_ - 1;
        const char *policyName = PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(policyIndex));
        fprintf(stdout, "Generating %zd password%s of length %zd using policy %s\n", count, count == 1 ? "" : "s",
                length, policyName);
        const auto passwords = GeneratePasswordCommand{app}.Execute();
        for (const auto &pw : passwords)
        {
            fprintf(stdout, "%s\n", pw.c_str());
        }
        fflush(stdout);
        break;
    }
    case Operation::EXPORT_DB: 
    {
        fprintf(stdout,
                "Exporting account database at %s\n"
                "Warning! The exported file will contain unencrypted passwords.\n"
                "You should securely delete the file when it is no longer needed.\n",
                args.output_file_.c_str());
        int rc = ExportDbCommand{app, args.output_file_}.Execute();
        if (rc != RC_SUCCESS)
        {
            result = static_cast<int>(RC_FAILURE);
            if (rc == RC_ERR_CANT_OPEN_FILE)
            {
                fprintf(stderr, "Error %d creating export file: %s\n", errno, strerror(errno));
            }
            else if (rc == RC_ERR_INCORRECT_PASSWORD)
            {
                fprintf(stderr, "An error occurred reading database file %s\n", args.database_.c_str());
            }
            else
            {
                fprintf(stderr, "An error occurred exporting the account database\n");
            }
        }
        fflush(stdout);
        break;
    }
    }

    return result;
}
