/* Copyright 2022 Ian Boisvert */
#pragma once

#include "ResultCode.h"

class PWSafeApp;

/** Generate a test account database */
class GenerateTestDbCommand
{
    PWSafeApp &m_app;

    ResultCode Generate(const std::string &database, const std::string &password, bool force, const std::string &language, 
        size_t groupCount, size_t itemCount) const;

public:
    GenerateTestDbCommand(PWSafeApp &app) : m_app(app) {}
    ResultCode Execute();
};
