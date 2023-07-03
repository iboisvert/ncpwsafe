/* Copyright 2022 Ian Boisvert */
#include <fcntl.h>
#include <unistd.h>

#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "GenerateTestDbCommand.h"
#include "Utils.h"
#include "RandUtils.h"

[[gnu::unused]] static const char *WORDS_EN_CA[] = {
#include "en_CA.dat"
};
static const size_t NWORDS_EN_CA = sizeof(WORDS_EN_CA) / sizeof(*WORDS_EN_CA);

[[gnu::unused]] static const char *WORDS_RU[] = {
#include "ru.dat"
};
static const size_t NWORDS_RU = sizeof(WORDS_RU) / sizeof(*WORDS_RU);

static void GetWordList(const std::string &lang, const char ***wordList, size_t &word_count)
{
    *wordList = nullptr;
    word_count = 0;
    if (lang.compare("en-CA") == 0)
    {
        *wordList = WORDS_EN_CA;
        word_count = NWORDS_EN_CA;
    }
    else if (lang.compare("ru") == 0)
    {
        *wordList = WORDS_RU;
        word_count = NWORDS_RU;
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

int GenerateTestDbCommand::Execute()
{
    if (generate_lang_.empty() || group_count_ < 1 || item_count_ < 1)
    {
        return RC_ERR_INVALID_ARG;
    }

    AccountDb &db = app_.GetDb();
    if (db.Exists() && !force_)
    {
        fprintf(stderr, "Error %d creating account database file: %s\n", errno, strerror(errno));
        return RC_FAILURE;
    }

    RandomState rand;

    const char **words;
    size_t word_count;
    GetWordList(generate_lang_, &words, word_count);

    std::vector<std::string> groups(group_count_);
    for (unsigned long i = 1; i < group_count_; ++i)
    {
        int size = rand() % 3 + 1;
        groups[i] = GeneratePhrase(words, word_count, size);
    }

    for (unsigned long i = 0; i < item_count_; ++i)
    {
        AccountRecord rec;

        int size = rand() % 3 + 1;
        rec.SetField(FT_TITLE, GeneratePhrase(words, word_count, size).c_str());
        int idx = rand() % group_count_;
        rec.SetField(FT_GROUP, groups[idx].c_str());
        rec.SetField(FT_USER, words[rand() % word_count]);
        rec.SetField(FT_PASSWORD, words[rand() % word_count]);
        if (rand() % 2)
        {
            size = rand() % 50 + 1;
            rec.SetField(FT_NOTES, GeneratePhrase(words, word_count, size).c_str());
        }

        db.Records().Add(std::move(rec));
    }

    int rc;
    db.WriteDb(&rc);

    return rc;
}
