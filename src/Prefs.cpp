/* Copyright 2023 Ian Boisvert */
#include <cstdlib>
#include <filesystem>

#include "libcpptoml.h"
#ifdef HAVE_GLOG
#include "libglog.h"
#endif

#include "Prefs.h"
#include "Utils.h"
#include "Filesystem.h"

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

template <class R>
R Prefs::GetPrefValue(const std::string &key)
{
    assert(prefs_->get_qualified(key)->is_value());
    return prefs_->get_qualified(key)->as<R>()->get();
}

template <>
std::string Prefs::GetPrefValue<std::string>(const std::string &key)
{
    assert(prefs_->get_qualified(key)->is_value());
    std::string value = prefs_->get_qualified(key)->as<std::string>()->get();
    value = ExpandEnvVars(value);
    return value;
}

template <class R>
R Prefs::Get(const std::string &key, const R &default_value)
{
    R retval = default_value;
    try
    {
        if (HasPref(key))
        {
            retval = GetPrefValue<R>(key);
        }
        else
        {
#ifdef HAVE_GLOG
            LOG(WARNING) << "No value in preferences for key \"" << key << "\"";
#endif
        }
        return retval;
    }
    catch(const std::exception& e)
    {
#ifdef HAVE_GLOG
        LOG(INFO) << "Exception retrieving preference value for key \"" << key << "\":" << e.what();
#endif
    }
    return retval;
}

// Template instantiation
template std::string Prefs::Get<std::string>(const std::string &, const std::string &);
template bool Prefs::Get<bool>(const std::string &, const bool &);

template <class R>
void Prefs::Set(const std::string &key, R value)
{
    assert(prefs_->get_qualified(key)->is_value());
    prefs_->insert(key, value);
}

// Template instantiation
template void Prefs::Set<std::string>(const std::string &, std::string);

bool Prefs::ReadPrefs(const std::string &pathname)
{
    bool retval = false;
    if (!fs::Exists(pathname))
    {
#ifdef HAVE_GLOG
        LOG(INFO) << "Preferences file \"" << pathname << "\" does not exist";
#endif
        goto done;
    }
    try
    {
        prefs_ = cpptoml::parse_file(pathname);
        if (!prefs_)
        {
#ifdef HAVE_GLOG
            LOG(WARNING) << "Error reading preferences from file \"" << pathname << "\"";
#endif
            goto done;
        }
    }
    catch(const std::exception& e)
    {
#ifdef HAVE_GLOG
        LOG(WARNING) << "Exception while reading preferences from file \"" << pathname << "\":" << e.what();
#endif
        goto done;
    }

    retval = true;

done:
    return retval;
}

bool Prefs::WritePrefs(const std::string &pathname)
{
    namespace fs = std::filesystem;
    fs::path config(pathname);
    std::error_code err;
    fs::create_directories(config.parent_path(), err);
    if (err.value() != 0) return false;

    std::ofstream ofs(config);
    ofs << (*prefs_);
    ofs.close();

    return true;
}
