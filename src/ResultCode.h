/* Copyright (c) 2022 Ian Boisvert */
#ifndef HAVE_RESULTCODE_H
#define HAVE_RESULTCODE_H

#include "libpwsafe.h"

enum ResultCode
{
    RC_SUCCESS = PRC_SUCCESS,
    RC_FAILURE = PRC_ERR_FAIL,
    RC_ERR_INCORRECT_PASSWORD = PRC_ERR_INCORRECT_PW,
    RC_ERR_INVALID_ARG = PRC_ERR_INVALID_ARG,
    RC_USER_CANCEL = 1024,
    RC_ERR_FILE_DOESNT_EXIST,
    RC_ERR_CANT_OPEN_FILE,
    RC_ERR_READONLY,
    RC_ERR_BACKUP
};

inline void SetResultCode(int *prc, int rc)
{
    if (prc) *prc = rc;
}

#endif