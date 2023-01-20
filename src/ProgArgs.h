/* Copyright 2020 Ian Boisvert */
#pragma once

#include <optional>

enum class Operation
{
    OPEN_DB,
    GENERATE_TEST_DB,
    GENERATE_PASSWORD,
    EXPORT_DB,
    CHANGE_DB_PASSWORD
};

struct InputProgArgs
{
    std::string m_progName;

    bool m_cmdGenerateTestDb = false;
    bool m_cmdGeneratePassword = false;
    bool m_cmdExportDb = false;
    bool m_cmdChangeDbPassword = false;

    bool m_force = false;                       // Force overwrite output file
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

/**
 * std::optional is a hassle to work with so
 * define a struct to use after args are parsed
 * that doesn't use std::optional
 */
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

    ProgArgs() = default;

    ProgArgs(const InputProgArgs &src)
    {
        m_progName = src.m_progName;
        m_command = src.GetCommand();
        m_force = src.m_force;
        // clang-format off
        if (src.m_database) m_database = *src.m_database;
        if (src.m_password) m_password = *src.m_password;
        if (src.m_newPassword) m_newPassword = *src.m_newPassword;
        if (src.m_readOnly) m_readOnly = *src.m_readOnly;
        if (src.m_generateLanguage) m_generateLanguage = *src.m_generateLanguage;
        if (src.m_generateGroupCount) m_generateGroupCount = *src.m_generateGroupCount;
        if (src.m_generateItemCount) m_generateItemCount = *src.m_generateItemCount;
        if (src.m_generatePasswordCount) m_generatePasswordCount = *src.m_generatePasswordCount;
        if (src.m_passwordPolicy) m_passwordPolicy = *src.m_passwordPolicy;
        if (src.m_passwordLength) m_passwordLength = *src.m_passwordLength;
        if (src.m_outputFile) m_file = *src.m_outputFile;
        // clang-format on
    }
};