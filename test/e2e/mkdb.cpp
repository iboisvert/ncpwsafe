/* Copyright 2026 Ian Boisvert */
/**
 * e2e-mkdb: build an account database with known contents for the end-to-end tests.
 *
 *   e2e-mkdb DATABASE_FILE PASSWORD_FILE SPEC_FILE
 *
 * SPEC_FILE has one account per line, tab-separated:
 *   group <TAB> title <TAB> user <TAB> password <TAB> notes
 * Trailing fields may be omitted, empty fields are left unset.
 * Blank lines and lines starting with '#' are ignored.
 *
 * The database is written with libpwsafe, the same code path as ncpwsafe,
 * so the tests don't depend on --test-db (Debug builds only) or on a
 * committed binary file.
 */
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "AccountDb.h"
#include "AccountRecord.h"
#include "Utils.h"

static std::vector<std::string> Split(const std::string &line, char sep)
{
    std::vector<std::string> fields;
    size_t pos = 0;
    while (true)
    {
        size_t idx = line.find(sep, pos);
        fields.push_back(line.substr(pos, idx == std::string::npos ? std::string::npos : idx - pos));
        if (idx == std::string::npos) break;
        pos = idx + 1;
    }
    return fields;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::fprintf(stderr, "Usage: %s DATABASE_FILE PASSWORD_FILE SPEC_FILE\n", argv[0]);
        return 2;
    }

    AccountDb db;
    db.DbPathname() = argv[1];
    if (!ReadPasswordFile(argv[2], db.Password()))
    {
        std::fprintf(stderr, "Unable to read password file %s\n", argv[2]);
        return 1;
    }

    std::ifstream spec(argv[3]);
    if (!spec)
    {
        std::fprintf(stderr, "Unable to read spec file %s\n", argv[3]);
        return 1;
    }

    static const uint8_t FIELDS[] = {FT_GROUP, FT_TITLE, FT_USER, FT_PASSWORD, FT_NOTES};
    std::string line;
    while (std::getline(spec, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> values = Split(line, '\t');
        AccountRecord rec;
        for (size_t i = 0; i < values.size() && i < sizeof(FIELDS); ++i)
        {
            rec.SetField(FIELDS[i], values[i].c_str());
        }
        db.Records().Save(rec);
    }

    int rc = 0;
    if (!db.WriteDb(&rc))
    {
        std::fprintf(stderr, "Error %d writing database %s\n", rc, argv[1]);
        return 1;
    }
    return 0;
}
