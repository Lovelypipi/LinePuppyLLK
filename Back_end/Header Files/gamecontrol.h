#ifndef GAMECONTROL_H
#define GAMECONTROL_H

#include "stdafx.h"

class GameControl{
public:
    //10行16列地图
    static const int MAP_ROWS = 10;
    static const int MAP_COLS = 16;
    //元素图片总数（Pic1-20）
    static constexpr int ICON_COUNT = 20;
    //基础模式实际参与消除的元素种类数
    static constexpr int BASIC_ICON_COUNT = 10;
    //单元格尺寸
    static const int CELL_WIDTH = 40;
    static const int CELL_HEIGHT = 50;
};

#endif