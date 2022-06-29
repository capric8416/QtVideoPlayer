#pragma once

#include <QObject>


class QOpenGLYUVWidget;
class QTimer;


class PlayTick : public QObject
{
	Q_OBJECT

public:
	PlayTick(QOpenGLYUVWidget **ppWidgets, int ways, int fps);
	~PlayTick();

	void Start();

public slots:
	void Play();

private:
	QTimer *m_pTimer;
	QOpenGLYUVWidget **m_ppWidgets;
	int m_nWays;
	int m_nFps;
};
