/* Copyright 2022 Ian Boisvert */
#include <fcntl.h>
#include <unistd.h>

#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "GenerateTestDbCommand.h"
#include "Utils.h"
#include "FileUtils.h"
#include "RandUtils.h"

[[gnu::unused]] static const char *WORDS_EN_CA[] = {
#include "en_CA.dat"
};
static const size_t NWORDS_EN_CA = sizeof(WORDS_EN_CA) / sizeof(*WORDS_EN_CA);

[[gnu::unused]] static const char *WORDS_RU[] = {
#include "ru.dat"
};
static const size_t NWORDS_RU = sizeof(WORDS_RU) / sizeof(*WORDS_RU);

static void GetWordList(const std::string &lang, const char ***wordList, size_t &wordCount)
{
    *wordList = nullptr;
    wordCount = 0;
    if (lang.compare("en-CA") == 0)
    {
        *wordList = WORDS_EN_CA;
        wordCount = NWORDS_EN_CA;
    }
    else if (lang.compare("ru") == 0)
    {
        *wordList = WORDS_RU;
        wordCount = NWORDS_RU;
    }
}

static std::string GeneratePhrase(const char *words[], unsigned wordsCount, unsigned phraseWordCount)
{
    RandomState rand;

    std::string result;
    const char *sep = "";
    for (unsigned i = 0; i < phraseWordCount; ++i)
    {
        int idx = rand() % wordsCount;
        result.append(sep).append(words[idx]);
        sep = " ";
    }
    return result;
}

ResultCode GenerateTestDbCommand::Generate(const std::string &db_pathname, const std::string &password, bool force,
                                           const std::string &language, size_t groupCount, size_t itemCount) const
{
    assert(!db_pathname.empty());
    assert(!language.empty());

    if (FileExists(db_pathname) && !force)
    {
        fprintf(stderr, "Error %d creating account database file: %s\n", errno, strerror(errno));
        return RC_FAILURE;
    }

    RandomState rand;

    const char **words;
    size_t wordCount;
    GetWordList(language, &words, wordCount);

    std::vector<std::string> groups(groupCount);
    for (unsigned long i = 1; i < groupCount; ++i)
    {
        int size = rand() % 3 + 1;
        groups[i] = GeneratePhrase(words, wordCount, size);
    }

    std::vector<std::unique_ptr<PwsDbRecord>> records;
    for (unsigned long i = 0; i < itemCount; ++i)
    {
        AccountRecord rec;

        int size = rand() % 3 + 1;
        rec.SetTitle(GeneratePhrase(words, wordCount, size).c_str());
        int idx = rand() % groupCount;
        rec.SetGroup(groups[idx].c_str());
        rec.SetUser(words[rand() % wordCount]);
        rec.SetPassword(words[rand() % wordCount]);
        if (rand() % 2)
        {
            size = rand() % 50 + 1;
            rec.SetNotes(GeneratePhrase(words, wordCount, size).c_str());
        }

        auto dbRecord = std::unique_ptr<PwsDbRecord>{rec.ToPwsDbRecord()};
        if (i > 0)
        {
            records[i-1]->next = dbRecord.get();
        }

        records.push_back(std::move(dbRecord));
    }

    PwsResultCode rc;
    AccountDb::WriteDb(db_pathname, password, records.front().get(), &rc);
    ResultCode result = StatusToRC(rc);

    return result;
}

ResultCode GenerateTestDbCommand::Execute()
{
    const ProgArgs &args = m_app.GetArgs();

    return Generate(args.m_database, args.m_password, args.m_force,
                    args.m_generateLanguage, args.m_generateGroupCount, args.m_generateItemCount);
}