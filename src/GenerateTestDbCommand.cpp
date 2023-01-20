/* Copyright 2022 Ian Boisvert */
#include <fcntl.h>
#include <unistd.h>

#include "core/PWSrand.h"
#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "GenerateTestDbCommand.h"
#include "Utils.h"

[[gnu::unused]]
static const wchar_t *WORDS_EN_CA[] = {
#include "en_CA.dat"
};
static const size_t NWORDS_EN_CA = sizeof(WORDS_EN_CA)/sizeof(*WORDS_EN_CA);

[[gnu::unused]]
static const wchar_t *WORDS_RU[] = {
#include "ru.dat"
};
static const size_t NWORDS_RU = sizeof(WORDS_RU) / sizeof(*WORDS_RU);

static void GetWordList(const stringT &lang, const wchar_t ***wordList, size_t &wordCount)
{
  *wordList = nullptr;
  wordCount = 0;
  if (lang.compare(L"en-CA") == 0)
  {
    *wordList = WORDS_EN_CA;
    wordCount = NWORDS_EN_CA;
  }
  else if (lang.compare(L"ru") == 0)
  {
    *wordList = WORDS_RU;
    wordCount = NWORDS_RU;
  }
}

static StringX GeneratePhrase(const wchar_t *words[], unsigned wordsCount, unsigned phraseWordCount)
{
  PWSrand *rand = PWSrand::GetInstance();

  StringX result;
  const wchar_t * sep = L"";
  for (unsigned i = 0; i < phraseWordCount; ++i)
  {
    int idx = rand->RandUInt() % wordsCount;
    result.append(sep).append(words[idx]);
    sep = L" ";
  }
  return result;
}

/** Create a new file, fails if file exists. */
static ResultCode CreateNewFile(cstringT &filename, bool force)
{
  int flags = O_WRONLY | O_CREAT;
  if (!force) flags = flags | O_EXCL;
  int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
  int fd = open(filename.c_str(), flags, mode);
  if (fd == -1)
  {
    fprintf(stderr, "Error %d creating account database file: %s\n", errno, strerror(errno));
    return ResultCode::FAILURE;
  }
  close(fd);

  return ResultCode::SUCCESS;
}

ResultCode GenerateTestDbCommand::Generate(const StringX &database, const StringX &password, bool force,
  const stringT &language, size_t groupCount, size_t itemCount) const
{
  assert(!database.empty());
  assert(!language.empty());

  PWScore & core = m_app.GetCore();
  PWSrand *rand = PWSrand::GetInstance();

  cstringT filename;
  WideToMultibyteString(database, filename);

  ResultCode result = CreateNewFile(filename, force);
  if (result != ResultCode::SUCCESS) return result;

  core.SetCurFile(database);
  core.NewFile(password);

  const wchar_t **words;
  size_t wordCount;
  GetWordList(language, &words, wordCount);

  const unsigned long NGROUP = groupCount;
  std::vector<StringX> groups(NGROUP);
  for (unsigned long i = 1; i < NGROUP; ++i)
  {
    int size = rand->RandUInt() % 3 + 1;
    groups[i] = GeneratePhrase(words, wordCount, size);
  }

  const unsigned long NITEM = itemCount;
  for (unsigned long i = 0; i < NITEM; ++i)
  {
    CItemData item;
    item.CreateUUID();

    int size = rand->RandUInt() % 3 + 1;
    item.SetTitle(GeneratePhrase(words, wordCount, size));
    int idx = rand->RandUInt() % NGROUP;
    item.SetGroup(groups[idx]);
    item.SetUser(words[rand->RandUInt() % wordCount]);
    item.SetPassword(words[rand->RandUInt() % wordCount]);
    if (rand->RandUInt() % 2)
    {
      size = rand->RandUInt() % 50 + 1;
      item.SetNotes(GeneratePhrase(words, wordCount, size));
    }

    // Add entry through command. This interface sucks for this
    // application, but it is better than making the entry list public.
    core.Execute(AddEntryCommand::Create(&core, item, pws_os::CUUID()));
  }

  result = StatusToRC(core.WriteCurFile());
  core.ReInit();

  return result;
}

ResultCode GenerateTestDbCommand::Execute()
{
    const ProgArgs &args = m_app.GetArgs();

    return Generate(args.m_database, args.m_password, args.m_force, 
        args.m_generateLanguage, args.m_generateGroupCount, args.m_generateItemCount);
}