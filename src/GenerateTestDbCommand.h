/* Copyright 2022 Ian Boisvert */
#pragma once

#include "ResultCode.h"

class PWSafeApp;

/** Generate a test account database */
class GenerateTestDbCommand
{
    PWSafeApp &m_app;

    ResultCode Generate(const StringX &database, const StringX &password, bool force, const stringT &language, 
        size_t groupCount, size_t itemCount) const;

public:
    GenerateTestDbCommand(PWSafeApp &app) : m_app(app) {}
    ResultCode Execute();
};
