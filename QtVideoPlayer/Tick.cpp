#include "Tick.h"
#include "YuvWidget.h"

#include <QTimer>


PlayTick::PlayTick(QOpenGLYUVWidget **ppWidgets, int ways, int fps)
	: m_pTimer(nullptr)
	, m_ppWidgets(ppWidgets)
	, m_nWays(ways)
	, m_nFps(fps)
{
	m_pTimer = new QTimer(this);
}


PlayTick::~PlayTick()
{
	if (m_pTimer != nullptr)
	{
		m_pTimer->stop();
		delete m_pTimer;
		m_pTimer = nullptr;
	}
}


void PlayTick::Start()
{
	// Æô¶¯¶¨Ê±Æ÷
	(void)connect(m_pTimer, SIGNAL(timeout()), this, SLOT(Play()));
	m_pTimer->start(1000 / m_nFps);
}

void PlayTick::Play()
{
	for (int i = 0; i < m_nWays; i++) {
		m_ppWidgets[i]->WakeTask();
	}
}