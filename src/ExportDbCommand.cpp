/* Copyright 2022 Ian Boisvert */
#include <cstdio>
#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "core/PWScore.h"
#include "ExportDbCommand.h"
#include "Utils.h"

/** Export account database to plaintext */
ResultCode ExportDbCommand::Execute()
{
    const ProgArgs &args = m_app.GetArgs();

    assert(!args.m_file.empty());

    PWScore &core = m_app.GetCore();
    core.SetReadOnly(true);
    int rc = core.ReadCurFile(args.m_password);
    if (rc != PWScore::SUCCESS)
    {
        return ResultCode::WRONG_PASSWORD;
    }

    cstringT filename;
    WideToMultibyteString(args.m_file, filename);

    FILE *f = fopen(filename.c_str(), "w");
    if (f == NULL)
    {
        return ResultCode::CANT_OPEN_FILE;
    }

    // Header
    fprintf(f, "UUID, Group, Title, User, Password, Notes\n");
    // Records
    AccountsColl &accounts = m_app.GetAccountsCollection();
    accounts.Refresh();
    for (const CItemData &entry : accounts)
    {
        uuid_array_t uuid;
        entry.GetUUID().GetARep(uuid);
        cstringT suuid;
        char buf[3];
        for (size_t i = 0; i < sizeof(uuid_array_t); ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                suuid.append("-");
            sprintf(buf, "%02x", uuid[i]);
            suuid.append(buf);
        }
        StringX notes = entry.GetNotes();
        notes = StringX{rtrim(notes.begin(), notes.end()), notes.end()};
        fprintf(f, "%s, \"%ls\", \"%ls\", \"%ls\", \"%ls\", \"%ls\"\n",
                suuid.c_str(), entry.GetGroup().c_str(), entry.GetTitle().c_str(),
                entry.GetUser().c_str(), entry.GetPassword().c_str(), notes.c_str());
    }

    fclose(f);

    return ResultCode::SUCCESS;
}
