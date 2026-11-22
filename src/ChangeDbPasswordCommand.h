/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_CHANGEDBPASSWORDCOMMAND_H
#define HAVE_CHANGEDBPASSWORDCOMMAND_H

#include <string>

class PWSafeApp;

/** Change the password of the account database */
class ChangeDbPasswordCommand
{
    PWSafeApp &app_;
    std::string new_password_;

public:
    ChangeDbPasswordCommand(PWSafeApp &app, std::string new_password) : app_{app}, new_password_{new_password} {}
    /** Read the database with its current password and write it with the new one, for --change-password */
    int Execute();
    /**
     * Write the records in memory with the new password, without reading the
     * database, and clear their dirty state. On failure the password is unchanged.
     * The current password must have been checked.
     */
    int Write();
};

#endif