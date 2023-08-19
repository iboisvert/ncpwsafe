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
#include "libglog.h"

#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <getopt.h>
#include <wchar.h>
#include <libgen.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

using std::fprintf;
using std::srand;

static bool DisableCoreDump()
{
#ifndef NDEBUG
#ifdef HAVE_SYS_PRCTL_H
    // prevent ptrace and creation of core dumps
    return prctl(PR_SET_DUMPABLE, 0) == 0;
#else
    return true;
#endif
#else
    return true;
#endif
}

static void Usage(const char *progName)
{
    using namespace std;

    std::string cfgfile = ExpandEnvVars(DEFAULT_CONFIG_FILE);

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
            "  -P,--password=STR    Account database password\n"
            "  --force              Force overwrite output file\n"
            "\n"
            "Open database options:\n"
            "  -r,--read-only       Open database as read-only\n"
            "\n"
            "Change account database options:\n"
            "  --new-password=STR   New account database password\n"
            "\n"
            "Generate test database options:\n"
            "  --test-db-lang=STR   Language of strings in generated database\n"
            "                       Valid values are en-CA, ru. Default value is %s\n"
            "  --test-db-groups=N   Number of groups in the generated database\n"
            "                       Default value is %zd\n"
            "  --test-db-items=N    Number of itesm in the generated database\n"
            "                       Default value is %zd\n"
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
            cfgfile.c_str(),
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

    size_t ncmds = args.cmd_generate_password_ ? 1 : 0 
            + args.cmd_export_db_ ? 1 : 0 
            + args.cmd_generate_test_db_ ? 1 : 0 
            + args.cmd_change_db_password_ ? 1 : 0;
    if (ncmds > 1)
    {
        result = Error("Only one command may be specified\n");
    }

    LOG_IF(INFO, args.config_file_ && !fs::Exists(*args.config_file_)) 
        << "Configuration file \"" << ExpandEnvVars(args.config_file_.value_or("")) << "\" does not exist";

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
            if (*args.password_policy_ > PasswordPolicy::Composition::COUNT)
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
        if (!args.force_ && fs::Exists(*args.output_file_))
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
    {"read-only", no_argument, nullptr, 'r'},
    {"generate-password", no_argument, nullptr, O_GENERATE_PASSWORD},
    {"password-count", required_argument, nullptr, O_PASSWORD_COUNT},
    {"password-policy", required_argument, nullptr, O_PASSWORD_POLICY},
    {"password-length", required_argument, nullptr, O_PASSWORD_LENGTH},
    {"export-db", no_argument, nullptr, O_EXPORT_DB},
    {"out", required_argument, nullptr, 'o'},
    {"change-password", no_argument, nullptr, O_CHANGE_PASSWORD},
    {"new-password", required_argument, nullptr, O_NEW_PASSWORD},
    {"force", no_argument, nullptr, OPT_FORCE},
    {"config", required_argument, nullptr, 'c'},
    {nullptr, 0, nullptr, 0},
};
// clang-format on

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
            char *pstr = optarg, *pend;
            size_t policy = strtoul(pstr, &pend, 10);
            if (policy == 0 && pend == pstr)
            {
                policy = (size_t)-1;
                for (size_t i = 0; i < PasswordPolicy::Composition::COUNT; ++i)
                {
                    const char *policyName = PasswordPolicy::GetName(static_cast<PasswordPolicy::Composition>(i));
                    if (strncmp(policyName, optarg, strlen(policyName)) == 0)
                    {
                        policy = i;
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

    bool status = ValidateArgs(args);
    return status;
}

int main(int argc, char *argv[])
{
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);
    
    // Don't allow ptrace or gdump on release build
    if (!DisableCoreDump())
        return false;

    InputProgArgs args;
    if (!ParseArgs(argc, argv, args))
    {
        Usage(basename(argv[0]));
        return PRC_ERR_FAIL;
    }

    srand(time(0));

    PWSafeApp app;
    app.Init(args);

    int result = static_cast<int>(RC_SUCCESS);
    switch (args.GetCommand())
    {
    case Operation::OPEN_DB: 
    {
        DialogResult rc = app.Show();
        result = static_cast<int>(rc == DialogResult::OK ? RC_SUCCESS : RC_FAILURE);
        app.SavePrefs();
        break;
    }
    case Operation::CHANGE_DB_PASSWORD: 
    {
        assert(args.new_password_);

        int rc = ChangeDbPasswordCommand{app, *args.new_password_}.Execute();
        if (rc == RC_SUCCESS)
        {
            fprintf(stdout, "Account database password changed\n");
        }
        else if (rc == RC_ERR_INCORRECT_PASSWORD)
        {
            fprintf(stdout, "Incorrect account database password\n");
        }
        else
        {
            fprintf(stdout, "Changing account database password failed\n");
        }
        fflush(stdout);
        break;
    }
    case Operation::GENERATE_TEST_DB:
    {
        assert(args.generate_language_);
        assert(args.generate_group_count_);
        assert(args.generate_item_count_);

        fprintf(stdout, "Generating test database at %s\n", args.database_->c_str());
        [[maybe_unused]] int rc = GenerateTestDbCommand{app, *args.force_,
            *args.generate_language_, *args.generate_group_count_, *args.generate_item_count_}.Execute();
        fflush(stdout);
        break;
    }
    case Operation::GENERATE_PASSWORD: 
    {
        size_t count = *args.generate_password_count_;
        size_t length = *args.password_length_;
        int policyIndex = *args.password_policy_ - 1;
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
        assert(args.output_file_);

        fprintf(stdout,
                "Exporting account database at %s\n"
                "Warning! The exported file will contain unencrypted passwords.\n"
                "You should securely delete the file when it is no longer needed.\n",
                args.output_file_->c_str());
        int rc = ExportDbCommand{app, *args.output_file_}.Execute();
        if (rc != RC_SUCCESS)
        {
            result = static_cast<int>(RC_FAILURE);
            if (rc == RC_ERR_CANT_OPEN_FILE)
            {
                fprintf(stderr, "Error %d creating export file: %s\n", errno, strerror(errno));
            }
            else if (rc == RC_ERR_INCORRECT_PASSWORD)
            {
                fprintf(stderr, "An error occurred reading database file %s\n", args.database_->c_str());
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
