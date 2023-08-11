/* Copyright 2020 Ian Boisvert */
#ifndef HAVE_PROGARGS_H
#define HAVE_PROGARGS_H

#include <optional>
#include <string>

enum class Operation
{
    OPEN_DB,
    GENERATE_TEST_DB,
    GENERATE_PASSWORD,
    EXPORT_DB,
    CHANGE_DB_PASSWORD
};

/** Arguments from CLI */
struct InputProgArgs
{
    std::string m_progName;
    bool m_cmdGenerateTestDb = false;
    bool m_cmdGeneratePassword = false;
    bool m_cmdExportDb = false;
    bool m_cmdChangeDbPassword = false;

    std::optional<bool> m_force;                       // Force overwrite output file
    std::optional<std::string> m_database;                 // Account database
    std::optional<std::string> m_password;                 // Database password
    std::optional<std::string> m_newPassword;              // New database password, for when password is being changed
    std::optional<bool> m_readOnly;                    // Database read-only flag
    std::optional<std::string> m_generateLanguage;         // Language to use in generated database
    std::optional<unsigned long> m_generateGroupCount; // Number of account groups to generate
    std::optional<unsigned long> m_generateItemCount;  // Number of account items to generate
    std::optional<size_t> m_generatePasswordCount;     // Number of passwords to generate
    std::optional<size_t> m_passwordPolicy;            // Policy used when generating passwords
    std::optional<size_t> m_passwordLength;            // Length of generated passwords
    std::optional<std::string> m_outputFile;                     // Target file, used for export database
    std::optional<std::string> config_file_;                     // Configuration file pathname

    Operation GetCommand() const
    {
        if (m_cmdGeneratePassword)
        {
            return Operation::GENERATE_PASSWORD;
        }
        else if (m_cmdGenerateTestDb)
        {
            return Operation::GENERATE_TEST_DB;
        }
        else if (m_cmdExportDb)
        {
            return Operation::EXPORT_DB;
        }
        else if (m_cmdChangeDbPassword)
        {
            return Operation::CHANGE_DB_PASSWORD;
        }
        else
        {
            return Operation::OPEN_DB;
        }
    }
};

extern const Operation DEFAULT_COMMAND;
extern const bool DEFAULT_FORCE;
extern const bool DEFAULT_READ_ONLY;
extern const char *DEFAULT_GENERATE_LANGUAGE;
extern const unsigned long DEFAULT_GENERATE_GROUP_COUNT;
extern const unsigned long DEFAULT_GENERATE_ITEM_COUNT;
extern const size_t DEFAULT_GENERATE_PASSWORD_COUNT;
extern const size_t DEFAULT_PASSWORD_POLICY;
extern const size_t DEFAULT_PASSWORD_LENGTH;
extern std::string DEFAULT_CONFIG_FILE;

struct ProgArgs
{
    std::string m_progName;
    Operation m_command;
    bool m_force;                       // Force overwrite output file
    std::string m_database;                 // Account database
    std::string m_password;                 // Database password
    std::string m_newPassword;  // New database password, for when password is being changed
    bool m_readOnly;                    // Database read-only flag
    std::string m_generateLanguage;         // Language to use in generated database
    unsigned long m_generateGroupCount; // Number of account groups to generate
    unsigned long m_generateItemCount;  // Number of account items to generate
    size_t m_generatePasswordCount;     // Number of passwords to generate
    size_t m_passwordPolicy;            // Policy used when generating passwords
    size_t m_passwordLength;            // Length of generated passwords
    std::string m_file;                     // Target file, used for export database
    std::string config_file_;  // Configuration file pathname

    /** Init options to defaults */
    ProgArgs() :
        m_command(DEFAULT_COMMAND),
        m_readOnly(DEFAULT_READ_ONLY),
        m_generateLanguage(DEFAULT_GENERATE_LANGUAGE),
        m_generateGroupCount(DEFAULT_GENERATE_GROUP_COUNT),
        m_generateItemCount(DEFAULT_GENERATE_ITEM_COUNT),
        m_generatePasswordCount(DEFAULT_GENERATE_PASSWORD_COUNT),
        m_passwordPolicy(DEFAULT_PASSWORD_POLICY),
        m_passwordLength(DEFAULT_PASSWORD_LENGTH),
        config_file_(DEFAULT_CONFIG_FILE)
    {
        // Empty
    }

    ProgArgs(const InputProgArgs &src)
    {
        m_progName = src.m_progName;
        m_command = src.GetCommand();
        // clang-format off
        if (src.m_force) m_force = src.m_force.value();
        if (src.m_database) m_database = src.m_database.value();
        if (src.m_password) m_password = src.m_password.value();
        if (src.m_newPassword) m_newPassword = src.m_newPassword.value();
        if (src.m_readOnly) m_readOnly = src.m_readOnly.value();
        if (src.m_generateLanguage) m_generateLanguage = src.m_generateLanguage.value();
        if (src.m_generateGroupCount) m_generateGroupCount = src.m_generateGroupCount.value();
        if (src.m_generateItemCount) m_generateItemCount = src.m_generateItemCount.value();
        if (src.m_generatePasswordCount) m_generatePasswordCount = src.m_generatePasswordCount.value();
        if (src.m_passwordPolicy) m_passwordPolicy = src.m_passwordPolicy.value();
        if (src.m_passwordLength) m_passwordLength = src.m_passwordLength.value();
        if (src.m_outputFile) m_file = src.m_outputFile.value();
        if (src.config_file_) config_file_ = src.config_file_.value();
        // clang-format on
    }
};

#endif  //#ifndef HAVE_PROGARGS_H
