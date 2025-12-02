// campusmapView.cpp : CcampusmapView 클래스의 구현
//

#include "pch.h"
#include "framework.h"

#ifndef SHARED_HANDLERS
#include "campusmap.h"
#endif

#include "campusmapDoc.h"
#include "campusmapView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif




IMPLEMENT_DYNCREATE(CcampusmapView, CView)

BEGIN_MESSAGE_MAP(CcampusmapView, CView)
    ON_WM_LBUTTONDOWN()     // 마우스 클릭을 통해 점/선/최단경로 기능 구현
END_MESSAGE_MAP()



CcampusmapView::CcampusmapView() noexcept
{
    m_edgeFirstNode = -1;   // Ctrl로 선을 만드는 변수
    m_pathStartNode = -1;   // Alt로 최단경로 구하는 변수
}

CcampusmapView::~CcampusmapView()
{
}

BOOL CcampusmapView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CView::PreCreateWindow(cs);
}



void CcampusmapView::OnInitialUpdate()
{
    CView::OnInitialUpdate();

    // 지도 이미지를 로딩하여 화면 출력 준비를 수행하는 부분
    m_bitmapMap.LoadBitmap(IDB_BITMAP4);

    BITMAP bmpInfo;
    m_bitmapMap.GetBitmap(&bmpInfo);

    // 불러온 이미지의 크기를 확보하는 과정
    m_mapSize.cx = bmpInfo.bmWidth;
    m_mapSize.cy = bmpInfo.bmHeight;
}




void CcampusmapView::OnDraw(CDC* pDC)
{

    // 지도 출력
    if (m_bitmapMap.GetSafeHandle())
    {
        // 지도 비트맵을 화면에 출력
        CDC memDC;
        memDC.CreateCompatibleDC(pDC);
        CBitmap* pOld = memDC.SelectObject(&m_bitmapMap);

        pDC->BitBlt(0, 0, m_mapSize.cx, m_mapSize.cy,
            &memDC, 0, 0, SRCCOPY);

        memDC.SelectObject(pOld);
    }

    CPen penBlue(PS_SOLID, 2, RGB(0, 0, 255));   // 파란 선
    CPen penRed(PS_SOLID, 3, RGB(255, 0, 0));    // 빨간 선
    CBrush brushBlue(RGB(0, 0, 255));            // 파란 점
    CBrush brushRed(RGB(255, 0, 0));             // 빨간 점

    int R = 5; // 점 반지름



    // 파란 선 (그래프 연결)

    pDC->SelectObject(&penBlue);
    pDC->SelectStockObject(NULL_BRUSH);

    for (int i = 0; i < (int)m_nodes.size(); ++i)
    {
        for (int j = i + 1; j < (int)m_nodes.size(); ++j)
        {
            // 점 두 개가 선택되면 그 두 점 사이에 파란 선을 그려 연결함
            if (m_adj.size() > i &&
                m_adj[i].size() > j &&
                m_adj[i][j] < INF)
            {
                pDC->MoveTo(m_nodes[i].pos);
                pDC->LineTo(m_nodes[j].pos);
            }
        }
    }


    // 파란 점 표시
    pDC->SelectObject(&brushBlue);

    for (auto& node : m_nodes)
    {
        // 파란 점 그리기
        CRect rc(node.pos.x - R, node.pos.y - R,
            node.pos.x + R, node.pos.y + R);
        pDC->Ellipse(rc);
    }


    // 빨간 최단경로
    if (m_shortestPath.size() >= 2)
    {
        pDC->SelectObject(&penRed);
        pDC->SelectObject(&brushRed);

        // 빨간 최단 경로 선
        for (int k = 0; k < (int)m_shortestPath.size() - 1; ++k)
        {
            int a = m_shortestPath[k];
            int b = m_shortestPath[k + 1];
            pDC->MoveTo(m_nodes[a].pos);
            pDC->LineTo(m_nodes[b].pos);
        }

        // 빨간 최단 경로 점
        for (int idx : m_shortestPath)
        {
            CRect rc(m_nodes[idx].pos.x - (R + 1),
                m_nodes[idx].pos.y - (R + 1),
                m_nodes[idx].pos.x + (R + 1),
                m_nodes[idx].pos.y + (R + 1));
            pDC->Ellipse(rc);
        }
    }
}




    // 마우스 클릭 처리
void CcampusmapView::OnLButtonDown(UINT nFlags, CPoint point)
{
    BOOL isCtrl = (nFlags & MK_CONTROL) != 0;
    BOOL isAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;


    //  최단 경로 계산 (dijkstra알고리즘 사용)
    if (isAlt)
    {
        int idx = FindNearestNode(point); //  ALT 클릭한 위치에서 가장 가까운 노드 찾기
        if (idx == -1) return;

        if (m_pathStartNode == -1)
        {
            m_pathStartNode = idx;      // 시작 노드 선택
            m_shortestPath.clear();    //  이전 빨간 경로 초기화
        }
        else
        {
            RunDijkstra(m_pathStartNode, idx);  // dijkstra알고리즘 실행
            m_pathStartNode = -1;
        }

        Invalidate();
        return;
    }


    //  CTRL: 두 노드를 선택하여 파란선 추가
    if (isCtrl)
    {
        int idx = FindNearestNode(point);   // 클릭된 위치와 가까운 점 찾기
        if (idx == -1) return;

        if (m_edgeFirstNode == -1)
        {
            m_edgeFirstNode = idx;         // 첫 번째 점 선택
        }
        else
        {
            int a = m_edgeFirstNode;
            int b = idx;
            m_edgeFirstNode = -1;

            if (a != b)
            {
                // 두 점 간 거리 계산 후 파란선 생성
                int dx = m_nodes[a].pos.x - m_nodes[b].pos.x;
                int dy = m_nodes[a].pos.y - m_nodes[b].pos.y;
                int dist = (int)sqrt(dx * dx + dy * dy);

                EnsureAdjSize();
                m_adj[a][b] = dist; // 연결된 거리값 저장
                m_adj[b][a] = dist;
            }
        }

        Invalidate();
        return;
    }


    //  기본 클릭: 새 점 생성
    Node node;
    node.id = (int)m_nodes.size();
    node.pos = point;

   //  새 점 추가 → 그래프의 정점 개수 증가
    m_nodes.push_back(node);
    EnsureAdjSize();

    Invalidate();
}



    // 인접행렬 크기 자동 조정
void CcampusmapView::EnsureAdjSize()
{
    // 그래프는 (노드 수 × 노드 수) 형태의 2차원 데이터로 저장됨
    int n = (int)m_nodes.size();
    m_adj.resize(n);

    for (int i = 0; i < n; ++i)
    {
        m_adj[i].resize(n, INF);   // 기본값은 연결 없음(INF)
        m_adj[i][i] = 0;    // 자기 자신까지의 거리는 0
    }
}



//     가장 가까운 노드 찾기
int CcampusmapView::FindNearestNode(CPoint pt, int threshold)
{
    // 클릭한 위치에서 가장 가까운 점 찾음
    int best = -1;
    int bestDist2 = threshold * threshold;

    for (int i = 0; i < (int)m_nodes.size(); ++i)
    {
        int dx = m_nodes[i].pos.x - pt.x;
        int dy = m_nodes[i].pos.y - pt.y;
        int d2 = dx * dx + dy * dy;

        if (d2 <= bestDist2)
        {
            bestDist2 = d2;
            best = i;
        }
    }
    return best;
}



// dijkstra 최단경로 알고리즘
void CcampusmapView::RunDijkstra(int startIdx, int endIdx)
{
    int n = (int)m_nodes.size();
    if (startIdx < 0 || endIdx < 0 ||
        startIdx >= n || endIdx >= n)
        return;

    // dist   : 시작점 → 각 점까지의 비용
    // prev   : 최단경로 역추적을 위한 이전 점 저장
    // visited: 방문 여부 체크
    std::vector<int> dist(n, INF);
    std::vector<int> prev(n, -1);
    std::vector<bool> visited(n, false);

    dist[startIdx] = 0;

    for (int iter = 0; iter < n; ++iter)
    {
        int u = -1;
        int best = INF;

        // 방문하지 않은 점 중 최단 거리 선택
        for (int i = 0; i < n; ++i)
            if (!visited[i] && dist[i] < best)
                best = dist[i], u = i;

        if (u == -1) break;
        if (u == endIdx) break;

        visited[u] = true;

        // 인접행렬을 이용하여 u와 연결된 이웃 노드들의 거리 갱신
        for (int v = 0; v < n; ++v)
        {
            if (m_adj[u][v] < INF)
            {
                int nd = dist[u] + m_adj[u][v];
                if (nd < dist[v])
                {
                    dist[v] = nd;
                    prev[v] = u;
                }
            }
        }
    }

    // ★ 최단 경로 역추적
    m_shortestPath.clear();
    if (dist[endIdx] == INF) return;

    int cur = endIdx;
    while (cur != -1)
    {
        m_shortestPath.push_back(cur);
        cur = prev[cur];
    }
    // 경로는 역방향으로 쌓이므로 뒤집어서 정방향으로 만들기
    std::reverse(m_shortestPath.begin(), m_shortestPath.end());
}


// 디버그
#ifdef _DEBUG
void CcampusmapView::AssertValid() const
{
    CView::AssertValid();
}

void CcampusmapView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif
