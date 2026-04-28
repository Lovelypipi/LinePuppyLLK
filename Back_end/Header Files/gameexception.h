#ifndef GAMEEXCEPTION_H
#define GAMEEXCEPTION_H

#include "stdafx.h"

class GameException : public exception{
public:
    explicit GameException(const char* msg);
    const char* what() const noexcept override;
private:
    const char* message;
};

#endif