#ifndef GAMEINFO_H
#define GAMEINFO_H

#include "xdcc.h"

struct GameInfo
{
    QString name;
    QString ip;
    QString players;
    QString id;
};

struct QueueInfo
{
    QString position;
    QString name;
};

struct PlayerInfo
{
    QString name;
    QString slot;
    QString realm;
    QString elo;
    QString games;
    QString kdr;
    QString wins;
};

#endif
