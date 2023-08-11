/* Copyright 2020 Ian Boisvert */
#include "ProgArgs.h"
#include "Utils.h"

const Operation DEFAULT_COMMAND = Operation::OPEN_DB;
const bool DEFAULT_FORCE = false;
const bool DEFAULT_READ_ONLY = false;
const char *DEFAULT_GENERATE_LANGUAGE = "en-CA";
const unsigned long DEFAULT_GENERATE_GROUP_COUNT = 10;
const unsigned long DEFAULT_GENERATE_ITEM_COUNT = 50;
const size_t DEFAULT_GENERATE_PASSWORD_COUNT = 1;
const size_t DEFAULT_PASSWORD_POLICY = 2;
const size_t DEFAULT_PASSWORD_LENGTH = 20;
std::string DEFAULT_CONFIG_FILE = ExpandEnvVars("${HOME}/.ncpwsafe.conf");
