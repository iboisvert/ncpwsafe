/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_GENERATEPASSWORDCOMMAND_H
#define HAVE_GENERATEPASSWORDCOMMAND_H

#include <vector>
#include <string>

class PWSafeApp;

/** Generate passwords to stdout */
class GeneratePasswordCommand
{
    PWSafeApp &m_app;

public:
    GeneratePasswordCommand(PWSafeApp &app) : m_app(app) {}
    /** Generate passwords to stdout */
    std::vector<std::string> Execute();
};

#endif