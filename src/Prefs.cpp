/* Copyright 2023 Ian Boisvert */
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "libconfini.h"
#ifdef HAVE_GLOG
#include "libglog.h"
#endif

#include "Prefs.h"
#include "Utils.h"
#include "Filesystem.h"

static std::map<std::string, std::string> DEFAULTS_{
    {Prefs::DB_PATHNAME, "${HOME}/.pwsafe.dat"},
    {Prefs::BACKUP_BEFORE_SAVE, "true"},
    {Prefs::BACKUP_COUNT, "3"},
};

Prefs Prefs::instance_;

/**
 * Initialize Prefs from defaults
*/
Prefs::Prefs() : prefs_(DEFAULTS_)
{
    // Empty
}

/** Returns `true` if preference exists, otherwise `false` */
bool Prefs::HasPref(const std::string &key)
{
    return (prefs_.find(key) != prefs_.end());
}

template <>
std::string Prefs::GetPrefValue<std::string>(const std::string &key)
{
    std::string value = prefs_.at(key);
    value = ExpandEnvVars(value);
    return value;
}

template <>
bool Prefs::GetPrefValue<bool>(const std::string &key)
{
    std::string &svalue = prefs_.at(key);
    int result = confini::ini_get_bool(svalue.c_str(), 0);
    bool retval = result ? true : false;
    return retval;
}

template <>
long Prefs::GetPrefValue<long>(const std::string &key)
{
    std::string &svalue = prefs_.at(key);
    long retval = confini::ini_get_lint(svalue.c_str());
    return retval;
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
    prefs_[key] = value;
}

// Template instantiation
template void Prefs::Set<std::string>(const std::string &, std::string);

int IniHandler(confini::IniDispatch *dispatch, void *user_data)
{
    using namespace confini;
    Prefs *prefs = reinterpret_cast<Prefs*>(user_data);
    assert(dispatch->type == IniNodeType::INI_KEY);
    std::string key = dispatch->data;
    std::string value = dispatch->value;
    prefs->prefs_[key] = value;
    return 0;
}

bool Prefs::ReadPrefs(const std::string &pathname)
{
    using namespace confini;

    bool retval = false;
    int status = confini::CONFINI_SUCCESS;

    if (!fs::Exists(pathname))
    {
#ifdef HAVE_GLOG
        LOG(INFO) << "Preferences file \"" << pathname << "\" does not exist";
#endif
        goto done;
    }

    status = load_ini_path(pathname.c_str(), INI_DEFAULT_FORMAT, /*finit*/nullptr, /*fforeach*/&IniHandler, /*user_data*/this);
    if (status != CONFINI_SUCCESS)
    {
#ifdef HAVE_GLOG
        LOG(WARNING) << "Error reading preferences from file \"" << pathname << "\"";
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
    if (err)
    {
#ifdef HAVE_GLOG
        LOG(WARNING) << "Error writing config file: " << err.message();
#endif
        return false;
    }

    std::ofstream ofs(config);
    std::for_each(prefs_.begin(), prefs_.end(), [&](auto it){
        ofs << it.first << " = " << it.second << '\n';
    });
    ofs.close();

    return true;
}
