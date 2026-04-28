#ifndef GAMECOMMON_H
#define GAMECOMMON_H

#include "gamecontrol.h"

struct Vertex{
    int row;
    int col;
    Vertex(int r = 0, int c = 0) : row(r), col(c) {}
};

struct GameModeConfig{
    int iconTypeCount;
    int mapRows;
    int mapCols;
    int maxTipCount;
    int maxResetCount;
    int timeLimitSeconds;

    GameModeConfig(int iconTypeCount_ = GameControl::BASIC_ICON_COUNT,
                                     int mapRows_ = GameControl::MAP_ROWS,
                                     int mapCols_ = GameControl::MAP_COLS,
                   int maxTipCount_ = 3,
                   int maxResetCount_ = 3,
                   int timeLimitSeconds_ = 180)
                : iconTypeCount(iconTypeCount_),
                    mapRows(mapRows_),
                    mapCols(mapCols_),
                    maxTipCount(maxTipCount_),
                    maxResetCount(maxResetCount_),
                    timeLimitSeconds(timeLimitSeconds_) {}
};

#endif