// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "Project2.h"
#include "ChildView.h"

#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	struct Vec3
	{
		float x;
		float y;
		float z;
	};

	Vec3 Normalize(Vec3 v)
	{
		const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		if (length <= 1e-6f)
		{
			return { 0.0f, 0.0f, 1.0f };
		}

		const float inv = 1.0f / length;
		return { v.x * inv, v.y * inv, v.z * inv };
	}

	float Clamp01(float v)
	{
		return (std::max)(0.0f, (std::min)(1.0f, v));
	}

	unsigned char ToByte(float v)
	{
		const int clamped = static_cast<int>(std::round((std::max)(0.0f, (std::min)(255.0f, v))));
		return static_cast<unsigned char>(clamped);
	}

	int ClampInt(int v, int lo, int hi)
	{
		return (std::max)(lo, (std::min)(hi, v));
	}
}

// CChildView

CChildView::CChildView()
	: m_gridWidth(220)
	, m_gridHeight(140)
	, m_waveSpeed(0.22f)
	, m_damping(0.993f)
	, m_disturbanceRadius(4)
	, m_paused(false)
	, m_obstacleEditMode(false)
	, m_renderMode(0)
	, m_obstacleLayout(1)
	, m_disturbancePreset(0)
	, m_frameCount(0)
	, m_pulsePhase(0.0f)
	, m_rng(static_cast<unsigned int>(::GetTickCount64()))
{
	ZeroMemory(&m_bitmapInfo, sizeof(m_bitmapInfo));
	InitializeSimulation(m_gridWidth, m_gridHeight);
	ApplyObstacleLayout(m_obstacleLayout);
}

CChildView::~CChildView()
{
}

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.style |= WS_TABSTOP;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);

	return TRUE;
}

int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	SetTimer(kSimTimerId, 16, nullptr);
	return 0;
}

void CChildView::OnDestroy()
{
	KillTimer(kSimTimerId);
	CWnd::OnDestroy();
}

BOOL CChildView::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CChildView::OnPaint()
{
	CPaintDC dc(this);
	CRect client;
	GetClientRect(&client);
	if (client.Width() <= 0 || client.Height() <= 0)
	{
		return;
	}

	RenderToPixelBuffer();

	StretchDIBits(
		dc.GetSafeHdc(),
		0,
		0,
		client.Width(),
		client.Height(),
		0,
		0,
		m_gridWidth,
		m_gridHeight,
		m_pixels.data(),
		&m_bitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY);

	DrawHud(dc);
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	UNREFERENCED_PARAMETER(cx);
	UNREFERENCED_PARAMETER(cy);
}

void CChildView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == kSimTimerId)
	{
		if (!m_paused)
		{
			StepSimulation();
		}
		Invalidate(FALSE);
	}

	CWnd::OnTimer(nIDEvent);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	const CPoint grid = ScreenToGrid(point);
	if (m_obstacleEditMode)
	{
		SetObstacleCircle(grid.x, grid.y, m_disturbanceRadius + 2, true);
	}
	else
	{
		DisturbAtCell(grid.x, grid.y, 1.0f, m_disturbanceRadius);
	}

	Invalidate(FALSE);
	CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	const CPoint grid = ScreenToGrid(point);
	if (m_obstacleEditMode)
	{
		SetObstacleCircle(grid.x, grid.y, m_disturbanceRadius + 2, false);
	}
	else
	{
		DisturbAtCell(grid.x, grid.y, -0.7f, m_disturbanceRadius);
	}

	Invalidate(FALSE);
	CWnd::OnRButtonDown(nFlags, point);
}

void CChildView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
	case VK_SPACE:
		m_paused = !m_paused;
		break;
	case 'R':
		ClearWater();
		break;
	case 'M':
		m_renderMode = (m_renderMode + 1) % 2;
		break;
	case 'O':
		m_obstacleEditMode = !m_obstacleEditMode;
		break;
	case 'L':
		m_obstacleLayout = (m_obstacleLayout + 1) % 4;
		ApplyObstacleLayout(m_obstacleLayout);
		break;
	case 'Q':
		m_waveSpeed = (std::min)(0.45f, m_waveSpeed + 0.01f);
		break;
	case 'A':
		m_waveSpeed = (std::max)(0.02f, m_waveSpeed - 0.01f);
		break;
	case 'W':
		m_damping = (std::min)(0.9995f, m_damping + 0.0005f);
		break;
	case 'S':
		m_damping = (std::max)(0.94f, m_damping - 0.0005f);
		break;
	case 'E':
		m_disturbanceRadius = (std::min)(20, m_disturbanceRadius + 1);
		break;
	case 'D':
		m_disturbanceRadius = (std::max)(1, m_disturbanceRadius - 1);
		break;
	case '1':
		m_disturbancePreset = 0;
		break;
	case '2':
		m_disturbancePreset = 1;
		break;
	case '3':
		m_disturbancePreset = 2;
		break;
	case '4':
		m_disturbancePreset = 3;
		break;
	default:
		break;
	}

	Invalidate(FALSE);
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

int CChildView::Index(int x, int y) const
{
	return (y * m_gridWidth) + x;
}

void CChildView::InitializeSimulation(int width, int height)
{
	m_gridWidth = width;
	m_gridHeight = height;

	const size_t cellCount = static_cast<size_t>(m_gridWidth * m_gridHeight);
	m_prevHeight.assign(cellCount, 0.0f);
	m_currHeight.assign(cellCount, 0.0f);
	m_nextHeight.assign(cellCount, 0.0f);
	m_obstacleMask.assign(cellCount, 0);
	m_pixels.assign(cellCount * 4, 0);

	ZeroMemory(&m_bitmapInfo, sizeof(m_bitmapInfo));
	m_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bitmapInfo.bmiHeader.biWidth = m_gridWidth;
	m_bitmapInfo.bmiHeader.biHeight = -m_gridHeight;
	m_bitmapInfo.bmiHeader.biPlanes = 1;
	m_bitmapInfo.bmiHeader.biBitCount = 32;
	m_bitmapInfo.bmiHeader.biCompression = BI_RGB;
}

void CChildView::ClearWater()
{
	std::fill(m_prevHeight.begin(), m_prevHeight.end(), 0.0f);
	std::fill(m_currHeight.begin(), m_currHeight.end(), 0.0f);
	std::fill(m_nextHeight.begin(), m_nextHeight.end(), 0.0f);
}

float CChildView::SampleHeightForWave(int cellIndex, int x, int y) const
{
	if (x < 0 || y < 0 || x >= m_gridWidth || y >= m_gridHeight)
	{
		return 0.0f;
	}

	const int neighbor = Index(x, y);
	if (m_obstacleMask[neighbor] != 0)
	{
		// Mirror the local value against obstacle cells for reflective behavior.
		return m_currHeight[cellIndex];
	}

	return m_currHeight[neighbor];
}

void CChildView::StepSimulation()
{
	++m_frameCount;

	if (m_disturbancePreset == 1)
	{
		std::uniform_int_distribution<int> xdist(2, m_gridWidth - 3);
		std::uniform_int_distribution<int> ydist(2, m_gridHeight - 3);
		for (int i = 0; i < 3; ++i)
		{
			DisturbAtCell(xdist(m_rng), ydist(m_rng), 0.35f, 1);
		}
	}
	else if (m_disturbancePreset == 2)
	{
		if ((m_frameCount % 42) == 0)
		{
			const int y = m_gridHeight / 3;
			for (int x = 2; x < m_gridWidth - 2; x += 2)
			{
				DisturbAtCell(x, y, 0.6f, 2);
			}
		}
	}
	else if (m_disturbancePreset == 3)
	{
		m_pulsePhase += 0.16f;
		DisturbAtCell(m_gridWidth / 2, m_gridHeight / 2, std::sin(m_pulsePhase) * 0.12f, 3);
	}

	for (int y = 0; y < m_gridHeight; ++y)
	{
		for (int x = 0; x < m_gridWidth; ++x)
		{
			const bool isBoundary = (x == 0 || y == 0 || x == (m_gridWidth - 1) || y == (m_gridHeight - 1));
			const int idx = Index(x, y);
			if (isBoundary || m_obstacleMask[idx] != 0)
			{
				m_nextHeight[idx] = 0.0f;
				continue;
			}

			const float center = m_currHeight[idx];
			const float left = SampleHeightForWave(idx, x - 1, y);
			const float right = SampleHeightForWave(idx, x + 1, y);
			const float up = SampleHeightForWave(idx, x, y - 1);
			const float down = SampleHeightForWave(idx, x, y + 1);

			const float laplacian = left + right + up + down - (4.0f * center);
			float next = (2.0f * center) - m_prevHeight[idx] + (m_waveSpeed * laplacian);
			next *= m_damping;
			m_nextHeight[idx] = next;
		}
	}

	m_prevHeight.swap(m_currHeight);
	m_currHeight.swap(m_nextHeight);
}

void CChildView::DisturbAtCell(int gx, int gy, float amplitude, int radius)
{
	for (int dy = -radius; dy <= radius; ++dy)
	{
		for (int dx = -radius; dx <= radius; ++dx)
		{
			const int x = gx + dx;
			const int y = gy + dy;
			if (x <= 0 || y <= 0 || x >= (m_gridWidth - 1) || y >= (m_gridHeight - 1))
			{
				continue;
			}

			const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
			if (dist > static_cast<float>(radius))
			{
				continue;
			}

			const int idx = Index(x, y);
			if (m_obstacleMask[idx] != 0)
			{
				continue;
			}

			const float falloff = 1.0f - (dist / static_cast<float>(radius + 1));
			m_currHeight[idx] += amplitude * falloff;
		}
	}
}

void CChildView::SetObstacleCircle(int gx, int gy, int radius, bool isObstacle)
{
	for (int dy = -radius; dy <= radius; ++dy)
	{
		for (int dx = -radius; dx <= radius; ++dx)
		{
			const int x = gx + dx;
			const int y = gy + dy;
			if (x <= 1 || y <= 1 || x >= (m_gridWidth - 2) || y >= (m_gridHeight - 2))
			{
				continue;
			}

			if ((dx * dx + dy * dy) > (radius * radius))
			{
				continue;
			}

			const int idx = Index(x, y);
			m_obstacleMask[idx] = isObstacle ? 1 : 0;
			m_prevHeight[idx] = 0.0f;
			m_currHeight[idx] = 0.0f;
			m_nextHeight[idx] = 0.0f;
		}
	}
}

void CChildView::AddRectObstacle(int left, int top, int right, int bottom)
{
	left = (std::max)(1, left);
	top = (std::max)(1, top);
	right = (std::min)(m_gridWidth - 2, right);
	bottom = (std::min)(m_gridHeight - 2, bottom);

	for (int y = top; y <= bottom; ++y)
	{
		for (int x = left; x <= right; ++x)
		{
			m_obstacleMask[Index(x, y)] = 1;
		}
	}
}

void CChildView::AddCircleObstacle(int cx, int cy, int radius)
{
	for (int y = (std::max)(1, cy - radius); y <= (std::min)(m_gridHeight - 2, cy + radius); ++y)
	{
		for (int x = (std::max)(1, cx - radius); x <= (std::min)(m_gridWidth - 2, cx + radius); ++x)
		{
			const int dx = x - cx;
			const int dy = y - cy;
			if ((dx * dx + dy * dy) <= (radius * radius))
			{
				m_obstacleMask[Index(x, y)] = 1;
			}
		}
	}
}

void CChildView::ApplyObstacleLayout(int layout)
{
	std::fill(m_obstacleMask.begin(), m_obstacleMask.end(), 0);

	switch (layout)
	{
	case 1:
		AddRectObstacle(m_gridWidth / 2 - 8, m_gridHeight / 2 - 20, m_gridWidth / 2 + 8, m_gridHeight / 2 + 20);
		AddRectObstacle(m_gridWidth / 4 - 4, m_gridHeight / 3, m_gridWidth / 4 + 4, m_gridHeight / 3 + 18);
		break;
	case 2:
		AddCircleObstacle(m_gridWidth / 3, m_gridHeight / 2, 12);
		AddCircleObstacle(2 * m_gridWidth / 3, m_gridHeight / 2, 16);
		AddCircleObstacle(m_gridWidth / 2, m_gridHeight / 3, 8);
		break;
	case 3:
		AddRectObstacle(m_gridWidth / 5, m_gridHeight / 4, m_gridWidth / 5 + 4, 3 * m_gridHeight / 4);
		AddRectObstacle(2 * m_gridWidth / 5, m_gridHeight / 6, 2 * m_gridWidth / 5 + 4, 2 * m_gridHeight / 3);
		AddRectObstacle(3 * m_gridWidth / 5, m_gridHeight / 3, 3 * m_gridWidth / 5 + 4, 5 * m_gridHeight / 6);
		AddRectObstacle(4 * m_gridWidth / 5, m_gridHeight / 5, 4 * m_gridWidth / 5 + 4, 4 * m_gridHeight / 5);
		break;
	default:
		break;
	}

	ClearWater();
}

float CChildView::SampleHeightForNormal(int x, int y) const
{
	if (x < 0 || y < 0 || x >= m_gridWidth || y >= m_gridHeight)
	{
		return 0.0f;
	}

	const int idx = Index(x, y);
	if (m_obstacleMask[idx] != 0)
	{
		return 0.0f;
	}

	return m_currHeight[idx];
}

void CChildView::RenderToPixelBuffer()
{
	const Vec3 lightDir = Normalize({ -0.55f, -0.35f, 1.0f });
	const Vec3 viewDir = { 0.0f, 0.0f, 1.0f };

	for (int y = 0; y < m_gridHeight; ++y)
	{
		for (int x = 0; x < m_gridWidth; ++x)
		{
			const int idx = Index(x, y);
			const int pixel = idx * 4;
			if (m_obstacleMask[idx] != 0)
			{
				m_pixels[pixel + 0] = 55;
				m_pixels[pixel + 1] = 55;
				m_pixels[pixel + 2] = 58;
				m_pixels[pixel + 3] = 255;
				continue;
			}

			const float hL = SampleHeightForNormal(x - 1, y);
			const float hR = SampleHeightForNormal(x + 1, y);
			const float hU = SampleHeightForNormal(x, y - 1);
			const float hD = SampleHeightForNormal(x, y + 1);
			const float h = m_currHeight[idx];

			const Vec3 normal = Normalize({ -(hR - hL) * 2.1f, -(hD - hU) * 2.1f, 1.0f });
			const float ndotl = (std::max)(0.0f, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);

			Vec3 reflect =
			{
				(2.0f * ndotl * normal.x) - lightDir.x,
				(2.0f * ndotl * normal.y) - lightDir.y,
				(2.0f * ndotl * normal.z) - lightDir.z
			};
			reflect = Normalize(reflect);

			const float specular = std::pow((std::max)(0.0f, reflect.z * viewDir.z), 32.0f);
			const float ambient = 0.21f;
			const float diffuse = 0.79f * ndotl;

			float red = 0.0f;
			float green = 0.0f;
			float blue = 0.0f;

			if (m_renderMode == 0)
			{
				const float depthTint = Clamp01((h * 0.5f) + 0.5f);
				red = (14.0f + (30.0f * depthTint)) * (ambient + diffuse) + (120.0f * specular);
				green = (66.0f + (80.0f * depthTint)) * (ambient + diffuse) + (130.0f * specular);
				blue = (126.0f + (95.0f * depthTint)) * (ambient + diffuse) + (160.0f * specular);
			}
			else
			{
				const float fresnel = std::pow(1.0f - Clamp01(normal.z), 3.5f);
				const float ripplePattern = Clamp01(0.5f + 0.5f * std::sin((x * 0.12f) + (y * 0.08f) + (h * 10.0f)));
				const float skyR = 126.0f;
				const float skyG = 167.0f;
				const float skyB = 216.0f;
				const float waterR = 8.0f + (20.0f * ripplePattern);
				const float waterG = 60.0f + (45.0f * ripplePattern);
				const float waterB = 110.0f + (55.0f * ripplePattern);

				red = ((1.0f - fresnel) * waterR + fresnel * skyR) * (ambient + diffuse) + (150.0f * specular);
				green = ((1.0f - fresnel) * waterG + fresnel * skyG) * (ambient + diffuse) + (150.0f * specular);
				blue = ((1.0f - fresnel) * waterB + fresnel * skyB) * (ambient + diffuse) + (180.0f * specular);
			}

			m_pixels[pixel + 0] = ToByte(blue);
			m_pixels[pixel + 1] = ToByte(green);
			m_pixels[pixel + 2] = ToByte(red);
			m_pixels[pixel + 3] = 255;
		}
	}
}

CPoint CChildView::ScreenToGrid(const CPoint& pt) const
{
	CRect client;
	GetClientRect(&client);
	const int clientW = (std::max)(1, client.Width());
	const int clientH = (std::max)(1, client.Height());

	const int gx = ClampInt((pt.x * m_gridWidth) / clientW, 0, m_gridWidth - 1);
	const int gy = ClampInt((pt.y * m_gridHeight) / clientH, 0, m_gridHeight - 1);
	return CPoint(gx, gy);
}

void CChildView::DrawHud(CDC& dc)
{
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(RGB(245, 245, 245));

	CString layoutName;
	switch (m_obstacleLayout)
	{
	case 0: layoutName = _T("None"); break;
	case 1: layoutName = _T("Pillars"); break;
	case 2: layoutName = _T("Circles"); break;
	default: layoutName = _T("Channel"); break;
	}

	CString presetName;
	switch (m_disturbancePreset)
	{
	case 0: presetName = _T("Manual clicks"); break;
	case 1: presetName = _T("Rain"); break;
	case 2: presetName = _T("Line splash"); break;
	default: presetName = _T("Pulse source"); break;
	}

	CString renderName = (m_renderMode == 0) ? _T("Lit water") : _T("Reflective tint");

	CString line1;
	line1.Format(_T("Space Pause:%s  |  L Layout:%s  |  O Obstacle Edit:%s  |  M Render:%s"),
		m_paused ? _T("ON") : _T("OFF"),
		layoutName.GetString(),
		m_obstacleEditMode ? _T("ON") : _T("OFF"),
		renderName.GetString());

	CString line2;
	line2.Format(_T("Q/A Wave: %.2f  W/S Damping: %.4f  E/D Radius: %d  R Reset"),
		m_waveSpeed,
		m_damping,
		m_disturbanceRadius);

	CString line3;
	line3.Format(_T("Presets 1-4: %s  |  Left click drop/add obstacle  |  Right click trough/remove obstacle"),
		presetName.GetString());

	dc.TextOutW(10, 10, line1);
	dc.TextOutW(10, 30, line2);
	dc.TextOutW(10, 50, line3);
}
