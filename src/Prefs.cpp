/* Copyright 2023 Ian Boisvert */
#include "Prefs.h"
#include <cstdlib>
#include "libcpptoml.h"
#include "libglog.h"
#include "FileUtils.h"
#include "Utils.h"

std::map<const char *, std::string> Prefs::DEFAULTS_{
    {Prefs::DB_PATHNAME, "${HOME}/.pwsafe.dat"},
    {Prefs::BACKUP_BEFORE_SAVE, "true"},
    {Prefs::BACKUP_COUNT, "3"},
};

Prefs Prefs::instance_;

/**
 * Initialize Prefs from defaults
*/
Prefs::Prefs()
{
    prefs_ = cpptoml::make_table();
    for (const auto &e : DEFAULTS_)
    {
        prefs_->insert(e.first, e.second);
    }
}

/** Returns `true` if preference exists, otherwise `false` */
bool Prefs::HasPref(const std::string &key)
{
    return prefs_->contains(key);
}

std::string Prefs::PrefAsString(const std::string &key)
{
    std::string retval;
    try
    {
        if (HasPref(key))
        {
            std::string str = prefs_->get_qualified(key)->as<std::string>()->get();
            str = ExpandEnvVars(str);
        }
        else
        {
            LOG(WARNING) << "No value in preferences for key \"" << key << "\"";
        }
        return retval;
    }
    catch(const std::exception& e)
    {
        LOG(INFO) << "Exception retrieving preference value for key \"" << key << "\":" << e.what();
    }
    return retval;
}

long Prefs::PrefAsInt(const std::string &key)
{
    long retval = 0;
    try
    {
        if (HasPref(key))
        {
            retval = prefs_->get_qualified(key)->as<long>()->get();
        }
        else
        {
            LOG(WARNING) << "No value in preferences for key \"" << key << "\"";
        }
    }
    catch(const std::exception& e)
    {
        LOG(INFO) << "Exception retrieving preference value for key \"" << key << "\":" << e.what();
    }
    return retval;
}

bool Prefs::PrefAsBool(const std::string &key)
{
    bool retval = false;
    try
    {
        if (HasPref(key))
        {
            retval = prefs_->get_qualified(key)->as<bool>()->get();
        }
        else
        {
            LOG(WARNING) << "No value in preferences for key \"" << key << "\"";
        }
    }
    catch(const std::exception& e)
    {
        LOG(INFO) << "Exception retrieving preference value for key \"" << key << "\":" << e.what();
    }
    return retval;
}

bool Prefs::ReadPrefs(const std::string &pathname)
{
    bool retval = false;
    if (!FileExists(pathname))
    {
        LOG(INFO) << "Preferences file \"" << pathname << "\" does not exist";
        goto done;
    }
    try
    {
        prefs_ = cpptoml::parse_file(pathname);
        if (!prefs_)
        {
            LOG(WARNING) << "Error reading preferences from file \"" << pathname << "\"";
            goto done;
        }
    }
    catch(const std::exception& e)
    {
        LOG(WARNING) << "Exception while reading preferences from file \"" << pathname << "\":" << e.what();
        goto done;
    }

    retval = true;

done:
    return retval;
}

bool Prefs::WritePrefs(const std::string &pathname)
{
    // TODO
    return false;
}
