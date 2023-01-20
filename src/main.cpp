/* Copyright 2020 Ian Boisvert */
#include "ExportDbCommand.h"
#include "ChangeDbPasswordCommand.h"
#include "GeneratePasswordCommand.h"
#include "GeneratePasswordDlg.h"
#include "GenerateTestDbCommand.h"
#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "Utils.h"
#include "libpwsafe.h"

#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <getopt.h>
#include <wchar.h>

using std::fprintf;
using std::srand;

static constexpr const char *DEFAULT_LANG = "en-CA";
static constexpr size_t DEFAULT_NGROUP = 10;
static constexpr size_t DEFAULT_NITEM = 50;
static constexpr size_t DEFAULT_NPASS = 1;
static constexpr size_t DEFAULT_PASS_LEN = 20;
static constexpr unsigned char DEFAULT_PASS_POLICY = 2;

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
            "  DATABASE_FILE        The account database file\n"
            "  -P,--password=STR    Account database password\n"
            "  -f,--force           Force overwrite output file\n"
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
            "  -o,--out=FILEPATH   Output file\n"
            "\n"
            "Help options:\n"
            "  -h,--help            Display this help text and exit\n",
            progName, 
            DEFAULT_LANG, DEFAULT_NGROUP, DEFAULT_NITEM,
            DEFAULT_NPASS,
            DEFAULT_PASS_LEN,
            GetPolicyName(0), GetPolicyName(1), GetPolicyName(2), GetPolicyName(3), GetPolicyName(4),
            GetPolicyName(DEFAULT_PASS_POLICY-1)
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

    size_t ncmds = args.m_cmdGeneratePassword ? 1 : 0 
            + args.m_cmdExportDb ? 1 : 0 
            + args.m_cmdGenerateTestDb ? 1 : 0 
            + args.m_cmdChangeDbPassword ? 1 : 0;
    if (ncmds > 1)
    {
        result = Error("Only one command may be specified\n");
    }
    Operation cmd = args.GetCommand();
    if (args.m_database)
    {
        if (cmd != Operation::OPEN_DB 
            && cmd != Operation::GENERATE_TEST_DB 
            && cmd != Operation::EXPORT_DB
            && cmd != Operation::CHANGE_DB_PASSWORD)
            result = false;
    }
    if (args.m_password)
    {
        if (cmd != Operation::OPEN_DB 
            && cmd != Operation::GENERATE_TEST_DB 
            && cmd != Operation::EXPORT_DB
            && cmd != Operation::CHANGE_DB_PASSWORD)
            result = false;
    }
    if (args.m_readOnly)
    {
        if (cmd != Operation::OPEN_DB && cmd != Operation::GENERATE_TEST_DB)
            result = false;
    }
    if (args.m_generateLanguage)
    {
        if (cmd != Operation::GENERATE_TEST_DB)
            result = false;
        else
        {
            if (args.m_generateLanguage->compare("en-CA") != 0 && args.m_generateLanguage->compare("ru") != 0)
                result = Error("Valid languages for test database are: en-CA, ru\n");
        }
    }
    if (args.m_generateGroupCount)
    {
        if (cmd != Operation::GENERATE_TEST_DB)
            result = false;
        else
        {
            if (*args.m_generateGroupCount < 1)
            {
                result = Error("Test account database group count must be > 0\n");
            }
        }
    }
    if (args.m_generateItemCount)
    {
        if (cmd != Operation::GENERATE_TEST_DB)
            result = false;
        else
        {
            if (*args.m_generateItemCount < 1)
            {
                result = Error("Test account database item count must be > 0\n");
            }
        }
    }
    if (args.m_generatePasswordCount)
    {
        if (cmd != Operation::GENERATE_PASSWORD)
            result = false;
        else
        {
            if (*args.m_generatePasswordCount < 1)
            {
                result = Error("Generated password count must be > 0\n");
            }
        }
    }
    if (args.m_passwordPolicy)
    {
        if (cmd != Operation::GENERATE_PASSWORD)
            result = false;
        else
        {
            if (*args.m_passwordPolicy > PW_POLICY_COUNT)
            {
                result = Error("Invalid policy for generated passwords\n");
            }
        }
    }
    if (args.m_passwordLength)
    {
        if (cmd != Operation::GENERATE_PASSWORD)
            result = false;
        else
        {
            if (*args.m_passwordLength < 1)
            {
                result = Error("Generated password length must be > 0\n");
            }
        }
    }

    if (cmd == Operation::GENERATE_TEST_DB)
    {
        if (!args.m_database || args.m_database->empty())
        {
            result = Error("Account database file is required\n");
        }
        else if (!args.m_force && pws_os::FileExists(args.m_database->c_str()))
        {
            result = Error("File %ls already exists\n", args.m_database->c_str());
        }
        if (!args.m_password || args.m_password->empty())
        {
            result = Error("Account database password is required\n");
        }
    }

    if (cmd == Operation::EXPORT_DB)
    {
        if (!args.m_database || args.m_database->empty())
        {
            result = Error("Account database file is required\n");
        }
        else if (!pws_os::FileExists(args.m_database->c_str()))
        {
            result = Error("Account database file %ls does not exist\n", args.m_database->c_str());
        }
        if (!args.m_password || args.m_password->empty())
        {
            result = Error("Account database password is required\n");
        }
        if (!args.m_outputFile || args.m_outputFile->empty())
        {
            result = Error("Output file is required\n");
        }
        if (!args.m_force && pws_os::FileExists(args.m_outputFile->c_str()))
        {
            result = Error("Output file %ls already exists\n", args.m_outputFile->c_str());
        }
    }

    if (cmd == Operation::CHANGE_DB_PASSWORD)
    {
        if (!args.m_database || args.m_database->empty())
        {
            result = Error("Account database file is required\n");
        }
        else if (!pws_os::FileExists(args.m_database->c_str()))
        {
            result = Error("Account database file %ls does not exist\n", args.m_database->c_str());
        }
        if (!args.m_password || args.m_password->empty())
        {
            result = Error("Account database password is required\n");
        }
        if (!args.m_newPassword || args.m_newPassword->empty())
        {
            result = Error("New account database password is required\n");
        }
    }
    return result;
}

void Normalize(InputProgArgs &args)
{
    // clang-format off
    if (!args.m_readOnly) args.m_readOnly = false;
    if (!args.m_generateLanguage) args.m_generateLanguage = DEFAULT_LANG;
    if (!args.m_generateGroupCount) args.m_generateGroupCount = DEFAULT_NGROUP;
    if (!args.m_generateItemCount) args.m_generateItemCount = DEFAULT_NITEM;
    if (!args.m_generatePasswordCount) args.m_generatePasswordCount = DEFAULT_NPASS;
    if (!args.m_passwordPolicy) args.m_passwordPolicy = DEFAULT_PASS_POLICY;
    if (!args.m_passwordLength) args.m_passwordLength = DEFAULT_PASS_LEN;
    // clang-format on
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
    O_NEW_PASSWORD
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
    {"force", no_argument, nullptr, 'f'},
    {nullptr, 0, nullptr, 0},
};
// clang-format on

static bool ParseArgs(int argc, char *const argv[], InputProgArgs &args)
{
    args.m_progName = argv[0];

    if (argc < 2)
        return true;

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "fo:P:r", options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'f': {
            args.m_force = true;
            break;
        }
        case O_GENERATE_PASSWORD: {
            args.m_cmdGeneratePassword = true;
            break;
        }
        case O_PASSWORD_COUNT: {
            assert(optarg);
            char *pstr = optarg, *pend;
            args.m_generatePasswordCount = strtoul(pstr, &pend, 10);
            break;
        }
        case O_PASSWORD_POLICY: {
            assert(optarg);
            char *pstr = optarg, *pend;
            size_t policy = strtoul(pstr, &pend, 10);
            if (policy == 0 && pend == pstr)
            {
                policy = (size_t)-1;
                for (size_t i = 0; i < PW_POLICY_COUNT; ++i)
                {
                    const char *policyName = GetPolicyName(i);
                    if (strncmp(policyName, optarg, strlen(policyName)) == 0)
                    {
                        policy = i;
                        break;
                    }
                }
            }
            args.m_passwordPolicy = policy;
            break;
        }
        case O_PASSWORD_LENGTH: {
            assert(optarg);
            args.m_passwordLength = strtoul(optarg, nullptr, 10);
            break;
        }
        case O_EXPORT_DB: {
            args.m_cmdExportDb = true;
            break;
        }
        case 'o': {
            assert(optarg);
            args.m_outputFile = optarg;
            break;
        }
        case O_CHANGE_PASSWORD: {
            args.m_cmdChangeDbPassword = true;
            break;
        }
        case O_NEW_PASSWORD: {
            assert(optarg);
            args.m_newPassword = optarg;
            break;
        }
        case O_GENERATE_TEST_DB: {
            args.m_cmdGenerateTestDb = true;
            break;
        }
        case O_TEST_DB_LANG: {
            assert(optarg);
            args.m_generateLanguage = optarg;
            break;
        }
        case O_TEST_DB_GROUPS: {
            assert(optarg);
            args.m_generateGroupCount = strtoul(optarg, nullptr, 10);
            break;
        }
        case O_TEST_DB_ITEMS: {
            assert(optarg);
            args.m_generateItemCount = strtoul(optarg, nullptr, 10);
            break;
        }
        case 'P': {
            assert(optarg);
            args.m_password = optarg;
            break;
        }
        case 'r': {
            args.m_readOnly = true;
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
        args.m_database = filename;
    }
    else if (count > 1)
    {
        return false;
    }

    if (ValidateArgs(args))
    {
        Normalize(args);
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    // Don't allow ptrace or gdump on release build
    if (!pws_os::DisableDumpAttach())
        return false;

    InputProgArgs args;
    if (!ParseArgs(argc, argv, args))
    {
        Usage(basename(argv[0]));
        return PWScore::FAILURE;
    }

    srand(time(0));

    PWSafeApp app;
    app.Init(args);

    int result = static_cast<int>(ResultCode::SUCCESS);
    switch (args.GetCommand())
    {
    case Operation::OPEN_DB: {
        DialogResult rc = app.Show();
        result = static_cast<int>(rc == DialogResult::OK ? ResultCode::SUCCESS : ResultCode::FAILURE);
        app.SavePrefs();
        break;
    }
    case Operation::CHANGE_DB_PASSWORD: {
        ResultCode rc = ChangeDbPasswordCommand{app, *args.m_database, *args.m_password, *args.m_newPassword}.Execute();
        result = static_cast<int>(rc);
        if (rc == ResultCode::SUCCESS)
        {
            fprintf(stdout, "Account database password changed\n");
        }
        else if (rc == ResultCode::WRONG_PASSWORD)
        {
            fprintf(stdout, "Incorrect account database password\n");
        }
        else
        {
            fprintf(stdout, "Changing account database password failed\n");
        }
        break;
    }
    case Operation::GENERATE_TEST_DB: {
        fprintf(stdout, "Generating test database at %s\n", args.m_database->c_str());
        ResultCode rc = GenerateTestDbCommand{app}.Execute();
        result = static_cast<int>(rc);
        fflush(stdout);
        break;
    }
    case Operation::GENERATE_PASSWORD: {
        size_t count = *args.m_generatePasswordCount;
        size_t length = *args.m_passwordLength;
        int policyIndex = *args.m_passwordPolicy - 1;
        const char *policyName = GetPolicyName(policyIndex);
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
    case Operation::EXPORT_DB: {
        fprintf(stdout,
                "Exporting account database at %s\n"
                "Warning! The exported file will contain unencrypted passwords.\n"
                "You should securely delete the file when it is no longer needed.\n",
                args.m_outputFile->c_str());
        ResultCode rc = ExportDbCommand{app}.Execute();
        if (rc != ResultCode::SUCCESS)
        {
            result = static_cast<int>(ResultCode::FAILURE);
            if (rc == ResultCode::CANT_OPEN_FILE)
            {
                fprintf(stderr, "Error %d creating export file: %s\n", errno, strerror(errno));
            }
            else if (rc == ResultCode::WRONG_PASSWORD)
            {
                fprintf(stderr, "An error occurred reading database file %s\n", args.m_database->c_str());
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
