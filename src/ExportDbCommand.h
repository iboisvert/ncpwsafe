/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_EXPORTDBCOMMAND_H
#define HAVE_EXPORTDBCOMMAND_H

#include "ResultCode.h"

class PWSafeApp;

/** Export account database to plaintext */
class ExportDbCommand
{
    PWSafeApp &app_;
    std::string &output_pathname_;

public:
    ExportDbCommand(PWSafeApp &app, std::string &output_pathname) : app_(app), output_pathname_{output_pathname} {}
    int Execute();
};

#endif