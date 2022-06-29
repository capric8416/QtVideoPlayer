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

		// 解析命令行参数
		parse(app);

		// 切分屏幕 - 决定启动 n x n 个播放器
		cut();

		// 测试模式，只启动一个1080p 30fps播放器

		char path[256];
		sprintf_s(path, "D:/risley/Media Files/H.264 Videos/Overwatch_Alive_Short_%dx%d.264", m_nWidth, m_nHeight);

		// 将264文件载入内存
		FileMemoryContext::Load(path);

		// 创建播放器
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

		// 启动定时器
		m_pTicker = new PlayTick(m_ppWidgets, m_nWays, m_nFps);
		m_pTicker->Start();
	}

	~ResourcesManager()
	{
		// 清理定时器
		delete m_pTicker;

		// 销毁播放器
		for (int i = 0; i < m_nWays; i++) {
			delete m_ppWidgets[i];
		}
		delete[] m_ppWidgets;

		// 清理264内存
		FileMemoryContext::Unload();

		qDebug() << "======================================== del ========================================";
	}

private:
	// 命令行解析
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

		// 测试模式
		m_bDebugMode = parser.isSet(debugOption);
		if (m_bDebugMode) {
			m_nWays = 1;
			m_nFps = 30;
		}
	}

	// 决定启动 n x n 个播放器
	void cut()
	{
		// 测试模式
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

	bool m_bDebugMode;  // 测试模式

	int m_nWays;  // 几路播放器
	int m_nFps;  // 帧率
	int m_nCount;  // 横向/纵向平铺几路播放器
	int m_nWidth;  // 单路播放器宽
	int m_nHeight;  // 单路播放器高

	PlayTick *m_pTicker;  // 定时器

	QOpenGLYUVWidget **m_ppWidgets;  // 窗口
};


