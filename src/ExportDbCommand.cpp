/* Copyright 2022 Ian Boisvert */
#include <cstdio>
#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "ExportDbCommand.h"
#include "Utils.h"

/** Export account database to plaintext */
ResultCode ExportDbCommand::Execute()
{
    const ProgArgs &args = m_app.GetArgs();

    assert(!args.m_file.empty());

    AccountDb &db = m_app.GetDb();
    db.ReadOnly() = true;
    int rc = db.ReadCurFile(args.m_password);
    if (rc != PWScore::SUCCESS)
    {
        return RC_ERR_WRONG_PASSWORD;
    }

    FILE *f = fopen(args.m_file.c_str(), "w");
    if (f == NULL)
    {
        return RC_ERR_CANT_OPEN_FILE;
    }

    // Header
    fprintf(f, "UUID, Group, Title, User, Password, Notes\n");
    // Records
    AccountsColl &accounts = m_app.GetAccountsCollection();
    accounts.Refresh();
    for (const AccountRecord &entry : accounts)
    {
        const char *uuid = entry.GetUUID();
        std::string suuid;
        char buf[3];
        for (size_t i = 0; i < 16; ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                suuid.append("-");
            sprintf(buf, "%02x", uuid[i]);
            suuid.append(buf);
        }
        std::string notes = entry.GetNotes();
        notes = std::string{rtrim(notes.begin(), notes.end()), notes.end()};
        fprintf(f, "%s, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"\n",
                suuid.c_str(), entry.GetGroup(), entry.GetTitle(),
                entry.GetUser(), entry.GetPassword(), notes.c_str());
    }

    fclose(f);

    return RC_SUCCESS;
}
