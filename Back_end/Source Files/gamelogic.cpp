#include "gamelogic.h"
#include "llkappsettings.h"

GameLogic::GameLogic()
    : m_pMap(NULL),
      m_rows(GameControl::MAP_ROWS),
    m_cols(GameControl::MAP_COLS),
    m_vexNum(0),
    m_nPathLen(0),
    m_nCorner(0) {}
GameLogic::~GameLogic(){
    ReleaseMap();
}

void GameLogic::InitMap(int iconTypeCount, int rows, int cols){
    if(rows <= 0 || cols <= 0){
        throw GameException("地图尺寸非法");
    }
    if((rows * cols) % 2 != 0){
        throw GameException("地图格子总数必须为偶数");
    }

    m_rows = rows;
    m_cols = cols;

    //分配内存
    m_pMap = new int* [m_rows];
    if(!m_pMap) throw GameException("地图内存分配失败");
    for(int i = 0; i < m_rows; i++){
        m_pMap[i] = new int[m_cols] {0};
        if(!m_pMap[i]) throw GameException("行内存分配失败");
    }
    //填充+打乱
    FillPairElements(iconTypeCount);
    ShuffleMap();

    //初始化图结构
    InitGraph();
    UpdateGraphAll();
}

void GameLogic::ReleaseMap(){
    if(m_pMap){
        for(int i = 0; i < m_rows; i++){
            delete[] m_pMap[i];
        }
        delete[] m_pMap;
        m_pMap = NULL;
    }
}

int GameLogic::GetMapVal(int row, int col){
    if(row < 0 || row >= m_rows || col < 0 || col >= m_cols){
        throw GameException("地图坐标越界");
    }
    return m_pMap[row][col];
}

void GameLogic::FillPairElements(int iconTypeCount){
    int total = m_rows * m_cols;
    const int validIconCount = std::max(1, std::min(iconTypeCount, GameControl::ICON_COUNT));
    int val = 1;
    for(int i = 0; i < total; i += 2){
        int row1 = i / m_cols;
        int col1 = i % m_cols;
        int row2 = (i + 1) / m_cols;
        int col2 = (i + 1) % m_cols;
        m_pMap[row1][col1] = val;
        m_pMap[row2][col2] = val;
        val = (val >= validIconCount) ? 1 : val + 1;
    }
}

void GameLogic::ShuffleMap(){
    for(int i = 0; i < m_rows; i++){
        for(int j = 0; j < m_cols; j++){
            int r = QRandomGenerator::global()->bounded(m_rows);
            int c = QRandomGenerator::global()->bounded(m_cols);
            swap(m_pMap[i][j], m_pMap[r][c]);
        }
    }
}

void GameLogic::ResetMap(int iconTypeCount, int rows, int cols){
    ReleaseMap();
    InitMap(iconTypeCount, rows, cols);
}

void GameLogic::ShuffleRemainingMap(){
    if(!m_pMap){
        return;
    }

    QVector<int> values;
    QVector<Vertex> positions;
    values.reserve(m_rows * m_cols);
    positions.reserve(m_rows * m_cols);

    for(int i = 0; i < m_rows; ++i){
        for(int j = 0; j < m_cols; ++j){
            values.push_back(m_pMap[i][j]);
            positions.push_back(Vertex(i, j));
        }
    }

    for(int i = values.size() - 1; i > 0; --i){
        int j = QRandomGenerator::global()->bounded(i + 1);
        swap(values[i], values[j]);
    }

    for(int i = 0; i < positions.size(); ++i){
        m_pMap[positions[i].row][positions[i].col] = values[i];
    }
    UpdateGraphAll();
}

void GameLogic::SetMapVal(int row, int col, int val){
    if(row < 0 || row >= m_rows || col < 0 || col >= m_cols){
        throw GameException("地图坐标越界");
    }
    m_pMap[row][col] = val;
    UpdateGraphNode(row, col, val);
}

//引擎一：线性结构
bool GameLogic::LinkInRow(int row, int col1, int col2){
    if(col1 > col2){
        swap(col1, col2);
    }
    for(int i = col1 + 1; i < col2; i++){
        if(m_pMap[row][i] != 0){ 
            return false;
        }
    }
    return true;
}

bool GameLogic::LinkInCol(int row1, int row2, int col){
    if(row1 > row2){
        swap(row1, row2);
    }
    for(int i = row1 + 1; i < row2; i++){
        if(m_pMap[i][col] != 0){
            return false;
        }
    }
    return true;
}

bool GameLogic::OneCornerLink(Vertex v1, Vertex v2, QVector<Vertex>& path){
    //(v1.row, v2.col) 先横后竖
    if(m_pMap[v1.row][v2.col] == 0){
        if(LinkInRow(v1.row, v1.col, v2.col) && LinkInCol(v1.row, v2.row, v2.col)){
            path.clear();
            path << v1 << Vertex(v1.row, v2.col) << v2;
            return true;
        }
    }
    //(v2.row, v1.col) 先竖后横
    if(m_pMap[v2.row][v1.col] == 0){
        if(LinkInCol(v1.row, v2.row, v1.col) && LinkInRow(v2.row, v1.col, v2.col)){
            path.clear();
            path << v1 << Vertex(v2.row, v1.col) << v2;
            return true;
        }
    }
    return false;
}

bool GameLogic::TwoCornerLink(Vertex v1, Vertex v2, QVector<Vertex>& path){
    //遍历所有列，尝试 横->竖->横 的路径
    for(int col = 0; col < m_cols; col++){
        if(LinkInRow(v1.row, v1.col, col) 
            && LinkInCol(v1.row, v2.row, col) 
            && LinkInRow(v2.row, col, v2.col)){
            if(m_pMap[v1.row][col] == 0 && m_pMap[v2.row][col] == 0){
                path.clear();
                path << v1 << Vertex(v1.row, col) << Vertex(v2.row, col) << v2;
                return true;
            }
        }
    }
    //遍历所有行，尝试 竖->横->竖 的路径
    for(int row = 0; row < m_rows; row++){
        if(LinkInCol(v1.row, row, v1.col) 
            && LinkInRow(row, v1.col, v2.col) 
            && LinkInCol(row, v2.row, v2.col)){
            if(m_pMap[row][v1.col] == 0 && m_pMap[row][v2.col] == 0){
                path.clear();
                path << v1 << Vertex(row, v1.col) << Vertex(row, v2.col) << v2;
                return true;
            }
        }
    }
    return false;
}

QVector<Vertex> GameLogic::GetPath(Vertex v1, Vertex v2){
    QVector<Vertex> path;
    Link(v1, v2, path);
    return path;
}

bool GameLogic::IsBlank(){
    for(int i = 0; i < m_rows; i++){
        for (int j = 0; j < m_cols; j++){
            if(m_pMap[i][j] != 0){
                return false;
            }
        }
    }
    return true;
}

int GameLogic::rowCount() const{
    return m_rows;
}

int GameLogic::colCount() const{
    return m_cols;
}

QVector<int> GameLogic::ExportMapValues() const{
    QVector<int> values;
    if(!m_pMap || m_rows <= 0 || m_cols <= 0){
        return values;
    }

    values.reserve(m_rows * m_cols);
    for(int r = 0; r < m_rows; ++r){
        for(int c = 0; c < m_cols; ++c){
            values.push_back(m_pMap[r][c]);
        }
    }
    return values;
}

bool GameLogic::ImportMapValues(int rows, int cols, const QVector<int>& values){
    if(rows <= 0 || cols <= 0){
        return false;
    }
    if(values.size() != rows * cols){
        return false;
    }

    ReleaseMap();
    m_rows = rows;
    m_cols = cols;

    m_pMap = new int*[m_rows];
    if(!m_pMap){
        return false;
    }
    for(int r = 0; r < m_rows; ++r){
        m_pMap[r] = new int[m_cols]{0};
        if(!m_pMap[r]){
            return false;
        }
    }

    int index = 0;
    for(int r = 0; r < m_rows; ++r){
        for(int c = 0; c < m_cols; ++c){
            m_pMap[r][c] = values[index++];
        }
    }

    InitGraph();
    UpdateGraphAll();
    return true;
}



//引擎二：非线性(图)结构
void GameLogic::InitGraph(){
    m_vexNum = m_rows * m_cols;
    m_nPathLen = 0;
    m_nCorner = 0;
    //初始化邻接矩阵
    for(int i = 0; i < 256; i++){
        m_vertices[i] = 0;
        for(int j = 0; j < 256; j++){
            m_adjMatrix[i][j] = false;
        }
    }
}

void GameLogic::UpdateGraphAll(){
    //先同步所有顶点值
    for(int r = 0; r < m_rows; r++){
        for(int c = 0; c < m_cols; c++){
            int u = ToIndex(Vertex(r, c));
            m_vertices[u] = m_pMap[r][c];
        }
    }

    //重建邻接关系
    for(int i = 0; i < m_vexNum; ++i){
        for(int j = 0; j < m_vexNum; ++j){
            m_adjMatrix[i][j] = false;
        }
    }
    for(int r = 0; r < m_rows; ++r){
        for(int c = 0; c < m_cols; ++c){
            UpdateArc(r, c);
        }
    }
}

void GameLogic::UpdateGraphNode(int r, int c, int val){
    int u = ToIndex(Vertex(r, c));
    if(u >= 0 && u < m_vexNum){
        m_vertices[u] = val;
        UpdateArc(r, c);
        if(r > 0) UpdateArc(r - 1, c);
        if(r < m_rows - 1) UpdateArc(r + 1, c);
        if(c > 0) UpdateArc(r, c - 1);
        if(c < m_cols - 1) UpdateArc(r, c + 1);
    }
}

void GameLogic::UpdateArc(int r, int c){
    if(r < 0 || r >= m_rows || c < 0 || c >= m_cols){
        return;
    }

    const int nV1Index = ToIndex(Vertex(r, c));
    const int dr[4] = {0, 0, -1, 1};
    const int dc[4] = {-1, 1, 0, 0};
    for(int i = 0; i < 4; i++){
        const int nr = r + dr[i];
        const int nc = c + dc[i];
        if(nr < 0 || nr >= m_rows || nc < 0 || nc >= m_cols){
            continue;
        }

        const int nV2Index = ToIndex(Vertex(nr, nc));
        m_adjMatrix[nV1Index][nV2Index] = false;
        m_adjMatrix[nV2Index][nV1Index] = false;
        if(CanArcLink(m_vertices[nV1Index], m_vertices[nV2Index])){
            m_adjMatrix[nV1Index][nV2Index] = true;
            m_adjMatrix[nV2Index][nV1Index] = true;
        }
    }
}

bool GameLogic::CanArcLink(int val1, int val2) const{
    //成边规则：相邻且(同图相连或至少一方为空)
    if(val1 == 0 || val2 == 0){
        return true;
    }
    return val1 == val2;
}

int GameLogic::ToIndex(const Vertex& v) const{
    return v.row * m_cols + v.col;
}

Vertex GameLogic::ToVertex(int index) const{
    return Vertex(index / m_cols, index % m_cols);
}

bool GameLogic::GraphLink(Vertex v1, Vertex v2, QVector<Vertex>& outPath){
    outPath.clear();
    const int start = ToIndex(v1);
    const int end = ToIndex(v2);
    if(start < 0 || start >= m_vexNum || end < 0 || end >= m_vexNum){
        return false;
    }

    m_nPathLen = 0;
    m_nCorner = 0;
    PushVertex(start);
    if(SearchPath(start, end)){
        for(int i = 0; i < m_nPathLen; ++i){
            outPath.push_back(ToVertex(m_anPath[i]));
        }
        return true;
    }

    m_nPathLen = 0;
    m_nCorner = 0;
    return false;
}

bool GameLogic::SearchPath(int nV0, int nV1){
    for(int nVi = 0; nVi < m_vexNum; nVi++){
        if(m_adjMatrix[nV0][nVi] && !IsExist(nVi)){
            //中间路径点必须为空；仅终点允许为非空节点
            if(nVi != nV1 && m_vertices[nVi] != 0){
                continue;
            }
            PushVertex(nVi);
            if(m_nCorner > 2){
                PopVertex();
                continue;
            }
            if(nVi == nV1){
                return true;
            }
            if(SearchPath(nVi, nV1)){
                return true;
            }
            PopVertex();
        }
    }
    return false;
}

void GameLogic::PushVertex(int nV){
    if(m_nPathLen >= 256){
        return;
    }
    m_anPath[m_nPathLen++] = nV;
    if(IsCorner()){
        ++m_nCorner;
    }
}

void GameLogic::PopVertex(){
    if(m_nPathLen <= 0){
        return;
    }
    if(IsCorner()){
        --m_nCorner;
    }
    --m_nPathLen;
}

bool GameLogic::IsExist(int nV) const{
    for(int i = 0; i < m_nPathLen; ++i){
        if(m_anPath[i] == nV){
            return true;
        }
    }
    return false;
}

bool GameLogic::IsCorner() const{
    if(m_nPathLen < 3){
        return false;
    }
    const int a = m_anPath[m_nPathLen - 3];
    const int b = m_anPath[m_nPathLen - 2];
    const int c = m_anPath[m_nPathLen - 1];
    return (b - a) != (c - b);
}

QVector<Vertex> GameLogic::ExtractCornerPath(const QVector<Vertex>& fullPath){
    if(fullPath.size() <= 2) return fullPath;
    QVector<Vertex> corners;
    corners.push_back(fullPath.first());
    
    for(int i = 1; i < fullPath.size() - 1; i++){
        Vertex prev = fullPath[i-1];
        Vertex curr = fullPath[i];
        Vertex next = fullPath[i+1];
        //若三个点不在同一直线上，说明这是个拐点
        if(!((prev.row == curr.row && curr.row == next.row) ||
             (prev.col == curr.col && curr.col == next.col))){
            corners.push_back(curr);
        }
    }
    corners.push_back(fullPath.last());
    return corners;
}

bool GameLogic::Link(Vertex v1, Vertex v2, QVector<Vertex>& path){
    path.clear();

    //统一入口校验，避免外部误调用导致两种引擎出现不一致的非法连通。
    if(v1.row < 0 || v1.row >= m_rows || v1.col < 0 || v1.col >= m_cols
        || v2.row < 0 || v2.row >= m_rows || v2.col < 0 || v2.col >= m_cols){
        return false;
    }
    const int val1 = m_pMap[v1.row][v1.col];
    const int val2 = m_pMap[v2.row][v2.col];
    if(val1 <= 0 || val2 <= 0 || val1 != val2){
        return false;
    }

    const LLKAppSettings::PathEngine engine = LLKAppSettings::instance().pathEngine();

    //设置为引擎一：只执行线性结构(代数规则枚举)
    if(engine == LLKAppSettings::EngineLinear){
        if(v1.row == v2.row && LinkInRow(v1.row, v1.col, v2.col)){
            path << v1 << v2;
            return true;
        }
        if(v1.col == v2.col && LinkInCol(v1.row, v2.row, v1.col)){
            path << v1 << v2;
            return true;
        }
        if(OneCornerLink(v1, v2, path)){
            return true;
        }
        if(TwoCornerLink(v1, v2, path)){
            return true;
        }
        path.clear();
        return false;
    }

    //设置为引擎二：只执行非线性结构(图深度优先搜索)
    QVector<Vertex> pathGraphFull;
    if(GraphLink(v1, v2, pathGraphFull)){
        path = ExtractCornerPath(pathGraphFull);
        return true;
    }
    path.clear();
    return false;
}