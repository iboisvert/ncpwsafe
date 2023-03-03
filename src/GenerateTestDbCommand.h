/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_GENERATETESTDBCOMMAND_H
#define HAVE_GENERATETESTDBCOMMAND_H

#include <string>

class PWSafeApp;

/** Generate a test account database */
class GenerateTestDbCommand
{
    PWSafeApp &app_;
    bool force_;
    std::string generate_lang_;
    size_t group_count_;
    size_t item_count_;

public:
    GenerateTestDbCommand(PWSafeApp &app, bool force, std::string generate_lang, size_t group_count, size_t item_count) : 
        app_(app), force_{force}, generate_lang_{generate_lang}, group_count_{group_count}, item_count_{item_count} {}
    int Execute();
};

#endif