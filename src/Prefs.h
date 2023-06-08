/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_PREFS_H
#define HAVE_PREFS_H

#include <string>
#include <map>
#include <memory>

namespace cpptoml
{
    class table;
}

struct Prefs
{
    static constexpr const char *DB_PATHNAME = "db-pathname";
    static constexpr const char *BACKUP_BEFORE_SAVE = "backup-before-save";
    static constexpr const char *BACKUP_COUNT = "backup-count";

    /**
     * Initialize Prefs from defaults
    */
    Prefs();

    /**
     * Initialize Prefs from defaults
     * then read settings from config file at `pathname`
     * \note
     * If file `pathname` does not exist, it is quietly ignored
    */
    Prefs(const std::string &pathname): Prefs()
    {
        ReadPrefs(pathname);
    }

    /** Singleton */
    static Prefs &Instance()
    {
        return instance_;
    }

    bool ReadPrefs(const std::string &pathname);
    bool WritePrefs(const std::string &pathname);

    /** Returns `true` if preference exists, otherwise `false` */
    bool HasPref(const std::string &key);

    /** 
     * Returns the value of the preference. 
     * 
     * If value for key does not exist, return empty string.
     * \see HasPref()
     */
    std::string PrefAsString(const std::string &key);
    /** 
     * Returns the value of the preference. 
     * 
     * If value for key does not exist, return `0`
     * \see HasPref()
     */
    long PrefAsInt(const std::string &key);
    /** 
     * Returns the value of the preference. 
     * 
     * If value for key does not exist, return `false`
     * \see HasPref()
     */
    bool PrefAsBool(const std::string &key);

private:
    static std::map<const char *, std::string> DEFAULTS_;
    static Prefs instance_;

    std::shared_ptr<cpptoml::table> prefs_;
};

#endif
