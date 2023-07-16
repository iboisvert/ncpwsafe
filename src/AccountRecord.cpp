/* Copyright 2023 Ian Boisvert */
#include "AccountRecord.h"
#include "libicu.h"

AccountRecord AccountRecord::FromPwsDbRecord(const PwsDbRecord *prec)
{
    assert(prec);

    AccountRecord rec;
    rec.SetField(FT_NAME, pws_rec_get_field(prec, FT_NAME));
    rec.SetField(FT_UUID, pws_rec_get_field(prec, FT_UUID));
    rec.SetField(FT_GROUP, pws_rec_get_field(prec, FT_GROUP));
    rec.SetField(FT_TITLE, pws_rec_get_field(prec, FT_TITLE));
    rec.SetField(FT_USER, pws_rec_get_field(prec, FT_USER));
    rec.SetField(FT_NOTES, pws_rec_get_field(prec, FT_NOTES));
    rec.SetField(FT_PASSWORD, pws_rec_get_field(prec, FT_PASSWORD));
    rec.SetField(FT_EMAIL, pws_rec_get_field(prec, FT_EMAIL));
    rec.SetField(FT_URL, pws_rec_get_field(prec, FT_URL));
    return rec;
}

/** 
 * Allocate field and add to linked list 
 * @param[in,out] phead Pointer to head of linked list of db fields
 */
static void CopyValueIntoFieldLinkedList(PwsDbField *&phead, uint8_t field_type, const std::string &value)
{
    if (!value.empty())
    {
        PwsDbField *pfield = new PwsDbField();
        pfield->type = field_type;
        pfield->value = const_cast<char *>(value.data());
        pfield->next = phead;
        phead = pfield;
    }
}

/**
 * Allocate and construct a PwsDbRecord struct from this.
 * Caller must `delete`.
 */
PwsDbRecord *AccountRecord::ToPwsDbRecord() const
{
    PwsDbRecord *prec = new PwsDbRecord();
    if (prec)
    {
        prec->next = nullptr;

        PwsDbField *&phead = prec->fields;
        for (auto &entry : m_fields)
        {
            CopyValueIntoFieldLinkedList(phead, entry.first, entry.second);
        }
    }
    return prec;
}

/** 
 * Returns `true` if the field exists and if the 
 * field value contains substring `substr`.
 * Substring is matched using case-insensitive comparison.
 */
bool AccountRecord::FieldContainsCaseInsensitive(uint8_t field_type, const std::string &substr) const
{
    const char *field = GetField(field_type);
    if (field)
    {
        icu::UnicodeString val(field);
        icu::UnicodeString sub(substr.c_str());
        if (val.toLower().indexOf(sub) != -1)
        {
            return true;
        }
    }
    return false;
}

/**
 * Compare a field in two `AccountRecord`s, return `true` if fields 
 * are equal.
 * If either field does not contain field `field_type`, returns `false`
*/
bool FieldCompare(uint8_t field_type, const AccountRecord &a, const AccountRecord &b)
{
    const char *fa = a.GetField(field_type);
    const char *fb = b.GetField(field_type);
    if (!fa || !fb)
    {
        return false;
    }

    return icu::UnicodeString(fa) == icu::UnicodeString(fb);
}
