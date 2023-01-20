/* Copyright 2022 Ian Boisvert */
#pragma once

#include "ResultCode.h"

class PWSafeApp;

/** Export account database to plaintext */
class ExportDbCommand
{
    PWSafeApp &m_app;

public:
    ExportDbCommand(PWSafeApp &app) : m_app(app) {}
    ResultCode Execute();
};
