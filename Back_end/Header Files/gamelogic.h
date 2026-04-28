#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include "stdafx.h"
#include "gamecommon.h"
#include "gamecontrol.h"
#include "gameexception.h"
#include <QVector>
#include <algorithm> 

class GameLogic{
public:
    GameLogic();
    ~GameLogic();

    void InitMap(int iconTypeCount = GameControl::BASIC_ICON_COUNT,
                 int rows = GameControl::MAP_ROWS,
                 int cols = GameControl::MAP_COLS);                  //初始化地图（成对元素+随机打乱）
    void ReleaseMap();                                                //释放地图内存
    int GetMapVal(int row, int col);                                 //获取地图值
    void ResetMap(int iconTypeCount = GameControl::BASIC_ICON_COUNT,
                  int rows = GameControl::MAP_ROWS,
                  int cols = GameControl::MAP_COLS);                 //重置地图
    void ShuffleRemainingMap();                                      //重排剩余元素
    void SetMapVal(int row, int col, int val);                       //设置地图单元格值
    QVector<Vertex> GetPath(Vertex v1, Vertex v2);                   //获取连线路径
    bool IsBlank();                                                  //判断地图是否已清空                                   
    int rowCount() const;
    int colCount() const;
    bool Link(Vertex v1, Vertex v2, QVector<Vertex>& path);               //总连接判断
    QVector<int> ExportMapValues() const;                                 //导出地图快照（按行展开）
    bool ImportMapValues(int rows, int cols, const QVector<int>& values); //导入地图快照

private:
    //引擎一：线性结构数据
    int** m_pMap;
    int m_rows;
    int m_cols;
    void FillPairElements(int iconTypeCount);                        //填充成对元素
    void ShuffleMap();                                               //随机打乱地图

    bool LinkInRow(int row, int col1, int col2);                     //同一行连通判断
    bool LinkInCol(int row1, int row2, int col);                     //同一列连通判断
    bool OneCornerLink(Vertex v1, Vertex v2, QVector<Vertex>& path); //单拐点连通判断(1折)
    bool TwoCornerLink(Vertex v1, Vertex v2, QVector<Vertex>& path); //双拐点连通判断(2折)

    //引擎二：非线性(图)结构数据
    int m_vertices[256];                                             //顶点
    bool m_adjMatrix[256][256];                                      //边
    int m_vexNum;                                                    //顶点总数
    int m_anPath[256];                                               //路径顶点索引栈
    int m_nPathLen;                                                  //当前路径长度
    int m_nCorner;                                                   //当前路径拐点数

    void InitGraph();                                                //初始化图的基本结构
    void UpdateGraphAll();                                           //同步全图数据
    void UpdateGraphNode(int r, int c, int val);                     //更新单个图节点
    void UpdateArc(int r, int c);                                    //更新单个节点邻接边
    bool CanArcLink(int val1, int val2) const;                       //判断两个顶点是否可连边
    int ToIndex(const Vertex& v) const;
    Vertex ToVertex(int index) const;
    
    bool GraphLink(Vertex v1, Vertex v2, QVector<Vertex>& path);
    bool SearchPath(int nV0, int nV1);
    void PushVertex(int nV);
    void PopVertex();
    bool IsExist(int nV) const;
    bool IsCorner() const;
    
    //将DFS走出的完整逐格路径，转化为UI需要的拐点关键帧路径
    QVector<Vertex> ExtractCornerPath(const QVector<Vertex>& fullPath);
};

#endif