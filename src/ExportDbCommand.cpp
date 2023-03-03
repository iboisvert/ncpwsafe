/* Copyright 2022 Ian Boisvert */
#include <cstdio>
#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "ExportDbCommand.h"
#include "Utils.h"

/** Export account database to plaintext */
int ExportDbCommand::Execute()
{
    assert(!output_pathname_.empty());

    AccountDb &db = app_.GetDb();

    int rc;
    if (!db.ReadDb(&rc))
    {
        return rc;
    }

    FILE *f = fopen(output_pathname_.c_str(), "w");
    if (!f)
    {
        return RC_ERR_CANT_OPEN_FILE;
    }
    std::unique_ptr<FILE, decltype(&fclose)> pf{f, &fclose};

    // Header
    fprintf(f, "UUID, Group, Title, User, Password, Notes\n");
    // Records
    auto &records = db.Records();
    for (const AccountRecord &rec : records)
    {
        const std::string &uuid = rec.GetField(FT_UUID);
        std::string suuid;
        char buf[3];
        for (size_t i = 0; i < 16; ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                suuid.append("-");
            sprintf(buf, "%02x", uuid[i]);
            suuid.append(buf);
        }
        std::string notes = rec.GetField(FT_NOTES);
        notes = std::string{rtrim(notes.begin(), notes.end()), notes.end()};
        fprintf(f, "%s, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"\n",
                suuid.c_str(), rec.GetField(FT_GROUP), rec.GetField(FT_TITLE),
                rec.GetField(FT_USER), rec.GetField(FT_PASSWORD), notes.c_str());
    }

    return RC_SUCCESS;
}
