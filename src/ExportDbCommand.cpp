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
    // A deleter type rather than decltype(&fclose), whose attributes (glibc's __nonnull)
    // a template argument can't keep (-Wignored-attributes)
    struct FileCloser
    {
        // Write errors aren't reported (docs/ISSUES.md)
        void operator()(FILE *file) const { (void)fclose(file); }
    };
    std::unique_ptr<FILE, FileCloser> pf{f};

    // Header
    fprintf(f, "UUID,Group,Title,User,Password,Notes\n");
    // Records
    auto &records = db.Records();
    for (const AccountRecord &rec : records)
    {
        fprintf(f, "%s,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                rec.GetField(FT_UUID, ""), rec.GetField(FT_GROUP, ""), rec.GetField(FT_TITLE, ""),
                rec.GetField(FT_USER, ""), rec.GetField(FT_PASSWORD, ""), rec.GetField(FT_NOTES, ""));
    }

    return RC_SUCCESS;
}
