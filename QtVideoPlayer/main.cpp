#include "QtVideoPlayer.h"
#include "FileMemoryContext.h"
#include "Tick.h"
#include "YuvWidget.h"

#include <QtWidgets/QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QScreen>
#include <QTimer>

#include <cmath>
#include <memory>


class ResourcesManager;


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	auto pResManager = std::make_unique<ResourcesManager>(app);

	return app.exec();
}


class ResourcesManager
{
public:
	ResourcesManager(QApplication &app)
		: m_bDebugMode(false)
		, m_nWays(1)
		, m_nFps(60)
		, m_nCount(1)
		, m_nWidth(1920)
		, m_nHeight(1080)

		, m_pTicker(nullptr)
		, m_ppWidgets()
	{
		qDebug() << "======================================== new ========================================";

		// ���������в���
		parse(app);

		// �з���Ļ - �������� n x n ��������
		cut();

		// ����ģʽ��ֻ����һ��1080p 30fps������

		char path[256];
		sprintf_s(path, "D:/risley/Media Files/H.264 Videos/Overwatch_Alive_Short_%dx%d.264", m_nWidth, m_nHeight);

		// ��264�ļ������ڴ�
		FileMemoryContext::Load(path);

		// ����������
		m_ppWidgets = new QOpenGLYUVWidget*[m_nWays];
		for (int i = 0; i < m_nCount; i++) {
			for (int j = 0; j < m_nCount; j++) {
				QOpenGLYUVWidget *pWidget = new QOpenGLYUVWidget();
				pWidget->setWindowFlag(Qt::FramelessWindowHint);
				pWidget->setGeometry(QRect(i * m_nWidth, j * m_nHeight, m_nWidth, m_nHeight));
				pWidget->show();
				m_ppWidgets[i * m_nCount + j] = pWidget;
			}
		}

		// ������ʱ��
		m_pTicker = new PlayTick(m_ppWidgets, m_nWays, m_nFps);
		m_pTicker->Start();
	}

	~ResourcesManager()
	{
		// ����ʱ��
		delete m_pTicker;

		// ���ٲ�����
		for (int i = 0; i < m_nWays; i++) {
			delete m_ppWidgets[i];
		}
		delete[] m_ppWidgets;

		// ����264�ڴ�
		FileMemoryContext::Unload();

		qDebug() << "======================================== del ========================================";
	}

private:
	// �����н���
	void parse(QApplication &app)
	{
		QCoreApplication::setApplicationName("QtVideoPlayer");
		QCoreApplication::setApplicationVersion("1.0");

		QCommandLineParser parser;
		parser.setApplicationDescription("Test FFmpeg decoder and OpenGL render");
		(void)parser.addHelpOption();
		(void)parser.addVersionOption();

		QCommandLineOption debugOption(QStringList() << "d" << "debug",
			QGuiApplication::translate("main", "Enter debug mode (default is false)."),
			QGuiApplication::translate("main", "debug"), "false");
		parser.addOption(debugOption);

		QCommandLineOption waysOption(QStringList() << "w" << "ways",
			QGuiApplication::translate("main", "How many players will be started (default is 4)."),
			QGuiApplication::translate("main", "ways"), "4");
		parser.addOption(waysOption);

		QCommandLineOption fpsOption(QStringList() << "f" << "fps",
			QGuiApplication::translate("main", "Frame rate per second (default is 60)."),
			QGuiApplication::translate("main", "fps"), "60");
		parser.addOption(fpsOption);

		parser.process(app);

		m_nWays = parser.value(waysOption).toInt();
		if (m_nWays > 16) {
			m_nWays = 16;
		}
		else if (m_nWays < 1) {
			m_nWays = 1;
		}

		m_nFps = parser.value(fpsOption).toInt();
		if (m_nFps > 60) {
			m_nFps = 60;
		}
		else if (m_nFps < 10) {
			m_nFps = 10;
		}

		// ����ģʽ
		m_bDebugMode = parser.isSet(debugOption);
		if (m_bDebugMode) {
			m_nWays = 1;
			m_nFps = 30;
		}
	}

	// �������� n x n ��������
	void cut()
	{
		// ����ģʽ
		if (m_bDebugMode) {
			m_nCount = 1;
			m_nWidth = 1920;
			m_nHeight = 1080;
		}
		else {
			QRect rect = QGuiApplication::primaryScreen()->geometry();

			m_nCount = sqrt(m_nWays);
			m_nWidth = rect.width() / m_nCount;
			m_nHeight = rect.height() / m_nCount;

		}
	}

	bool m_bDebugMode;  // ����ģʽ

	int m_nWays;  // ��·������
	int m_nFps;  // ֡��
	int m_nCount;  // ����/����ƽ�̼�·������
	int m_nWidth;  // ��·��������
	int m_nHeight;  // ��·��������

	PlayTick *m_pTicker;  // ��ʱ��

	QOpenGLYUVWidget **m_ppWidgets;  // ����
};


