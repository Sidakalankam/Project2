// ChildView.h : interface of the CChildView class
//

#pragma once

#include <random>
#include <vector>

// CChildView window

class CChildView : public CWnd
{
// Construction
public:
	CChildView();

// Attributes
public:

// Operations
public:

// Overrides
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CChildView();

protected:
	static constexpr UINT_PTR kSimTimerId = 1;

	int m_gridWidth;
	int m_gridHeight;
	std::vector<float> m_prevHeight;
	std::vector<float> m_currHeight;
	std::vector<float> m_nextHeight;
	std::vector<unsigned char> m_obstacleMask;
	std::vector<unsigned char> m_pixels;
	BITMAPINFO m_bitmapInfo;

	float m_waveSpeed;
	float m_damping;
	int m_disturbanceRadius;
	bool m_paused;
	bool m_obstacleEditMode;
	int m_renderMode;
	int m_obstacleLayout;
	int m_disturbancePreset;
	int m_frameCount;
	float m_pulsePhase;
	std::mt19937 m_rng;

	int Index(int x, int y) const;
	void InitializeSimulation(int width, int height);
	void ClearWater();
	void StepSimulation();
	void DisturbAtCell(int gx, int gy, float amplitude, int radius);
	void SetObstacleCircle(int gx, int gy, int radius, bool isObstacle);
	void ApplyObstacleLayout(int layout);
	void AddRectObstacle(int left, int top, int right, int bottom);
	void AddCircleObstacle(int cx, int cy, int radius);
	void RenderToPixelBuffer();
	CPoint ScreenToGrid(const CPoint& pt) const;
	float SampleHeightForNormal(int x, int y) const;
	float SampleHeightForWave(int cellIndex, int x, int y) const;
	void DrawHud(CDC& dc);

	// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()
};
