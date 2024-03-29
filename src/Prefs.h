/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_PREFS_H
#define HAVE_PREFS_H

#include <string>
#include <map>
#include <memory>

namespace confini
{
    struct IniDispatch;
}

class Prefs
{
public:
    /** Account database file pathname, string */
    static constexpr const char *DB_PATHNAME = "db-pathname";
    /** 
     * Backup account database file before saving, boolean.
     * If `true`, backup of account database will be created before saving.
     * If `false`, no backup will be created, existing backups will not be removed.
     */
    static constexpr const char *BACKUP_BEFORE_SAVE = "backup-before-save";
    /** 
     * Count of backup database files to maintain, integer.
     * Backup database files older than count will be removed 
     * when a new backup is saved.
     */
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
     * If value for key does not exist, returns `default_value`.
     * \see HasPref()
     */
    template <class R>
    R Get(const std::string &key, const R &default_value);

    /**
     * Returns the value of the preference. 
     * 
     * \throws If value for key does not exist
    */
    template <class R>
    R GetPrefValue(const std::string &key);

    /** Set the value of a preference */
    template <class R>
    void Set(const std::string &key, R value);

    void Delete(const std::string &key);
private:
    static Prefs instance_;

    std::map<std::string, std::string> prefs_;

    friend int IniHandler(confini::IniDispatch *dispatch, void *user_data);
};

#endif
