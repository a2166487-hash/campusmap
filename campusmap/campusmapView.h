// campusmapView.h : CcampusmapView 클래스의 인터페이스
//

#pragma once

#include <vector>
#include <algorithm>
#include <math.h>

class CcampusmapDoc;   

class CcampusmapView : public CView
{
protected:
    CcampusmapView() noexcept;
    DECLARE_DYNCREATE(CcampusmapView)

public:
    //   그래프 자료구조 (캠퍼스 지도 위의 길 표현)
    //
    // - 학교 캠퍼스 그림을 배경으로 사용
    // - 마우스로 교차로 위치에 점을 찍어서 추가
    // - Ctrl 키를 누른 채 점 두 개를 선택하면 두 점 사이를 파란선 으로 연결
    // - Alt 키를 누른 채 점 두 개를 선택하면 그 두 점 사이의 최단경로를 Dijkstra 알고리즘으로 계산
    // - 점과 일반 길은 파란색, 최단경로의 점과 선은 빨간색으로 표시
    // - 여러 점과 간선을 만들면 전체가 하나의 그래프가 됨
    // - 그래프는 점과 파란선으로 구성되고,
    //   노드 사이의 거리 정보는 2차원 인접행렬로 저장됨
    //   예) 1번과 2번 사이의 거리가 40이면 m_adj[1][2] = 40 이런 식으로 저장
    struct Node
    {
        int   id;   // 점 번호 (0, 1, 2, ...)
        CPoint pos; // 화면 상에서의 좌표 (교차로 위치)
    };

    // 사용자가 클릭해서 만든 모든 점 목록
    std::vector<Node> m_nodes;

    // 인접행렬: m_adj[i][j] = i번 점에서 j번 점까지의 거리
    // - 두 점이 파란선으로 연결되어 있지 않으면 INF 값 사용
    // - 예) 1번과 2번 사이의 거리가 40이면 m_adj[1][2] = 40, m_adj[2][1] = 40
    std::vector<std::vector<int>> m_adj;

    // Dijkstra 알고리즘으로 계산한 최단경로에 포함된 점 인덱스들
    // - Alt로 시작점과 도착점을 선택하면 이 벡터에 경로가 저장되고
    //   OnDraw에서 빨간 점/빨간 선으로 표시됨
    std::vector<int> m_shortestPath;

    // 파란선이 없음을 표시하기 위한 매우 큰 값 (INF)
    static const int INF = 1000000000;

    int m_edgeFirstNode;  // Ctrl 모드: 첫 번째로 선택한 점 인덱스 (두 번째 점 선택 시 파란선 생성)
    int m_pathStartNode;  // Alt 모드: 최단경로 출발 점 인덱스 (두 번째 점 선택 시 Dijkstra 알고리즘 실행)

    //   캠퍼스 지도 비트맵
    CBitmap m_bitmapMap;  // 배경으로 사용할 캠퍼스 지도 비트맵
    CSize   m_mapSize;    // 지도 비트맵의 가로/세로 크기



    // 그래프 데이터를 (노드 수 × 노드 수) 형태의 2차원 데이터로 유지
    void EnsureAdjSize();

    // 마우스로 클릭한 위치 근처에서 가장 가까운 점을 찾는 함수
    int  FindNearestNode(CPoint pt, int threshold = 15);

    // Dijkstra 알고리즘을 이용해 startIdx → endIdx까지의 최단경로를 계산하는 함수
    void RunDijkstra(int startIdx, int endIdx);

public:
    // 화면 그리기
    // - 캠퍼스 지도 비트맵 출력
    // - 파란 점과 파란 선그리기
    // - m_shortestPath에 저장된 경로는 빨간 점/빨간 선으로 최단경로 표시
    virtual void OnDraw(CDC* pDC);

    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    // 뷰가 처음 생성될 때 한 번 호출
    virtual void OnInitialUpdate();

protected:
    virtual ~CcampusmapView();

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif


    //메시지 맵
protected:
    // 마우스 왼쪽 버튼 클릭 처리
    // - 그냥 클릭  : 새 점 추가
    // - Ctrl + 클릭: 두 점을 선택해서 파란 길 생성
    // - Alt + 클릭 : 두 점을 선택해서 Dijkstra 최단경로 계산 및 빨간색 경로 표시
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  
inline CcampusmapDoc* CcampusmapView::GetDocument() const
{
    return reinterpret_cast<CcampusmapDoc*>(m_pDocument);
}
#endif