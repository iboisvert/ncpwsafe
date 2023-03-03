/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_CHANGEDBPASSWORDCOMMAND_H
#define HAVE_CHANGEDBPASSWORDCOMMAND_H

#include <string>

class PWSafeApp;

/** Export account database to plaintext */
class ChangeDbPasswordCommand
{
    PWSafeApp &app_;
    std::string new_password_;

public:
    ChangeDbPasswordCommand(PWSafeApp &app, std::string new_password) : app_{app}, new_password_{new_password} {}
    int Execute();
};

#endif