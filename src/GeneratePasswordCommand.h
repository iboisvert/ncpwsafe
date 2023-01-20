/* Copyright 2022 Ian Boisvert */
#pragma once
#include <vector>
#include <core/StringX.h>

class PWSafeApp;

/** Generate passwords to stdout */
class GeneratePasswordCommand
{
    PWSafeApp &m_app;

public:
    GeneratePasswordCommand(PWSafeApp &app) : m_app(app) {}
    /** Generate passwords to stdout */
    std::vector<StringX> Execute();
};
