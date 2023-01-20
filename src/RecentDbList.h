/* Copyright (c) 2022 Ian Boisvert */
#pragma once

#include "core/PWSprefs.h"

class RecentDbList
{
private:
    std::vector<stringT> m_mruList;

public:
    size_t GetCount()
    {
        return m_mruList.size();
    }

    const stringT &GetHistoryFile(size_t idx)
    {
        return m_mruList[idx];
    }

    void AddFileToHistory(const stringT &file)
    {
        m_mruList.push_back(file);
    }

    void RemoveFileFromHistory(size_t idx)
    {
        m_mruList.erase(m_mruList.begin() + idx);
    }

    void RemoveFile(const stringT &file)
    {
        for (size_t idx = 0, max = GetCount(); idx < max; ++idx)
        {
            if (GetHistoryFile(idx) == file)
            {
                RemoveFileFromHistory(idx);
                return;
            }
        }
    }

    void Save() const
    {
        PWSprefs::GetInstance()->SetMRUList(&m_mruList[0], static_cast<int>(m_mruList.size()),
                                            PWSprefs::GetInstance()->GetPref(PWSprefs::MaxMRUItems));
    }

    void Load()
    {
        PWSprefs *prefs = PWSprefs::GetInstance();
        const auto nExpected = prefs->GetPref(PWSprefs::MaxMRUItems);
        std::vector<stringT> mruList(nExpected);
        __attribute__((unused)) const auto nFound = prefs->GetMRUList(&mruList[0]);
        assert(nExpected >= nFound);
        m_mruList = mruList;
    }

    void Clear()
    {
        m_mruList.clear();
    }
};
