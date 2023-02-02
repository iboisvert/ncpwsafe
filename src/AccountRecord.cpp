/* Copyright 2023 Ian Boisvert */
#include "AccountRecord.h"

AccountRecord AccountRecord::FromPwsDbRecord(const PwsDbRecord *prec)
{
    assert(prec);

    AccountRecord rec;
    rec.SetName(pws_rec_get_field(prec, FT_NAME));
    rec.SetUUID(pws_rec_get_field(prec, FT_UUID));
    rec.SetGroup(pws_rec_get_field(prec, FT_GROUP));
    rec.SetTitle(pws_rec_get_field(prec, FT_TITLE));
    rec.SetUser(pws_rec_get_field(prec, FT_USER));
    rec.SetNotes(pws_rec_get_field(prec, FT_NOTES));
    rec.SetPassword(pws_rec_get_field(prec, FT_PASSWORD));
    rec.SetEmail(pws_rec_get_field(prec, FT_EMAIL));
    rec.SetURL(pws_rec_get_field(prec, FT_URL));
    return rec;
}

static void CopyValueIntoFieldLinkedList(PwsDbField *phead, uint8_t field_type, std::string &value)
{
    if (!value.empty())
    {
        PwsDbField *pfield = new PwsDbField();
        pfield->type = field_type;
        pfield->value = value.data();
        pfield->next = phead;
        phead = pfield;
    }
}

/**
 * Allocate and construct a PwsDbRecord struct from this.
 * Caller must `delete`.
 */
PwsDbRecord *AccountRecord::ToPwsDbRecord()
{
    PwsDbRecord *prec = new PwsDbRecord();
    prec->next = nullptr;

    PwsDbField *&phead = prec->fields;
    for (auto &entry : m_fields)
    {
        CopyValueIntoFieldLinkedList(phead, entry.first, entry.second);
    }
}
