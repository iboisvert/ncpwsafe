/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_GENERATEPASSWORDCOMMAND_H
#define HAVE_GENERATEPASSWORDCOMMAND_H

#include <vector>
#include <string>

class PWSafeApp;

/** Generate passwords to stdout */
class GeneratePasswordCommand
{
    PWSafeApp &app_;

public:
    GeneratePasswordCommand(PWSafeApp &app) : app_(app) {}
    /** Generate passwords to stdout */
    std::vector<std::string> Execute();
};

#endif