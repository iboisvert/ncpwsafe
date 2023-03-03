/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_PREFS_H
#define HAVE_PREFS_H

#include <string>
#include <map>
#include <memory>
#include "cpptoml.h"

struct Prefs
{
    static constexpr const char *DB_PATHNAME = "db-pathname";
    static constexpr const char *BACKUP_BEFORE_SAVE = "backup-before-save";
    static constexpr const char *BACKUP_COUNT = "backup-count";

    static Prefs &Instance()
    {
        return instance;
    }

    bool WritePrefs(const std::string &pathname);
    bool WritePrefs();

    std::string &Pref(const char *key);
    int PrefAsInt(const char *key);
    bool PrefAsBool(const char *key);

private:
    static Prefs instance;
    static std::map<const char *, std::string> DEFAULTS;

    std::map<std::string, std::string> prefs;
};

#endif
