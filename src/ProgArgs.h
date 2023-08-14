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
    std::string prog_name_;
    bool cmd_generate_test_db_ = false;
    bool cmd_generate_password_ = false;
    bool cmd_export_db_ = false;
    bool cmd_change_db_password_ = false;

    std::optional<bool> force_;                       // Force overwrite output file
    std::optional<std::string> database_;                 // Account database
    std::optional<std::string> password_;                 // Database password
    std::optional<std::string> new_password_;              // New database password, for when password is being changed
    std::optional<bool> read_only_;                    // Database read-only flag
    std::optional<std::string> generate_language_;         // Language to use in generated database
    std::optional<unsigned long> generate_group_count_; // Number of account groups to generate
    std::optional<unsigned long> generate_item_count_;  // Number of account items to generate
    std::optional<size_t> generate_password_count_;     // Number of passwords to generate
    std::optional<size_t> password_policy_;            // Policy used when generating passwords
    std::optional<size_t> password_length_;            // Length of generated passwords
    std::optional<std::string> output_file_;                     // Target file, used for export database
    std::optional<std::string> config_file_;                     // Configuration file pathname

    Operation GetCommand() const
    {
        if (cmd_generate_password_)
        {
            return Operation::GENERATE_PASSWORD;
        }
        else if (cmd_generate_test_db_)
        {
            return Operation::GENERATE_TEST_DB;
        }
        else if (cmd_export_db_)
        {
            return Operation::EXPORT_DB;
        }
        else if (cmd_change_db_password_)
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
    std::string prog_name_;
    Operation command_;
    bool force_;                       // Force overwrite output file
    std::string database_;                 // Account database
    std::string password_;                 // Database password
    std::string new_password_;  // New database password, for when password is being changed
    bool read_only_;                    // Database read-only flag
    std::string generate_language_;         // Language to use in generated database
    unsigned long generate_group_count_; // Number of account groups to generate
    unsigned long generate_item_count_;  // Number of account items to generate
    size_t generate_password_count_;     // Number of passwords to generate
    size_t password_policy_;            // Policy used when generating passwords
    size_t password_length_;            // Length of generated passwords
    std::string output_file_;                     // Target file, used for export database
    std::string config_file_;  // Configuration file pathname

    /** Init options to defaults */
    ProgArgs() :
        command_(DEFAULT_COMMAND),
        read_only_(DEFAULT_READ_ONLY),
        generate_language_(DEFAULT_GENERATE_LANGUAGE),
        generate_group_count_(DEFAULT_GENERATE_GROUP_COUNT),
        generate_item_count_(DEFAULT_GENERATE_ITEM_COUNT),
        generate_password_count_(DEFAULT_GENERATE_PASSWORD_COUNT),
        password_policy_(DEFAULT_PASSWORD_POLICY),
        password_length_(DEFAULT_PASSWORD_LENGTH),
        config_file_(DEFAULT_CONFIG_FILE)
    {
        // Empty
    }

    ProgArgs(const InputProgArgs &src) :
        ProgArgs()
    {
        prog_name_ = src.prog_name_;
        command_ = src.GetCommand();
        // clang-format off
        if (src.force_) force_ = src.force_.value();
        if (src.database_) database_ = src.database_.value();
        if (src.password_) password_ = src.password_.value();
        if (src.new_password_) new_password_ = src.new_password_.value();
        if (src.read_only_) read_only_ = src.read_only_.value();
        if (src.generate_language_) generate_language_ = src.generate_language_.value();
        if (src.generate_group_count_) generate_group_count_ = src.generate_group_count_.value();
        if (src.generate_item_count_) generate_item_count_ = src.generate_item_count_.value();
        if (src.generate_password_count_) generate_password_count_ = src.generate_password_count_.value();
        if (src.password_policy_) password_policy_ = src.password_policy_.value();
        if (src.password_length_) password_length_ = src.password_length_.value();
        if (src.output_file_) output_file_ = src.output_file_.value();
        if (src.config_file_) config_file_ = src.config_file_.value();
        // clang-format on
    }
};

#endif  //#ifndef HAVE_PROGARGS_H
