/* Copyright (c) 2022 Ian Boisvert */
#pragma once

enum class ResultCode
{
    SUCCESS = 0,
    FAILURE,
    USER_CANCEL,
    FILE_DOESNT_EXIST,
    WRONG_PASSWORD,
    CANT_OPEN_FILE
};

inline ResultCode StatusToRC(int status)
{
    switch (status)
    {
    case PWScore::SUCCESS: return ResultCode::SUCCESS;
    case PWScore::FAILURE: return ResultCode::FAILURE;
    case PWScore::USER_CANCEL: return ResultCode::USER_CANCEL;
    case PWScore::CANT_OPEN_FILE: return ResultCode::CANT_OPEN_FILE;
    case PWScore::WRONG_PASSWORD: return ResultCode::WRONG_PASSWORD;
    default:
        return ResultCode::FAILURE;
    }
}