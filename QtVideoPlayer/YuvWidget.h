#pragma once

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QFile>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>


#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

class VideoDecoder;

class QOpenGLYUVWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    QOpenGLYUVWidget(QWidget* parent = nullptr);
    ~QOpenGLYUVWidget() Q_DECL_OVERRIDE;

	void StartTask();  // 启动后台解码渲染线程
	void StopTask();   // 停止并销毁后台解码渲染线程
	void WakeTask();   // 唤醒一次后台解码渲染工作流

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;

private:
	void StartDecoder();
	void StopDecoder();

    // 纹理是一个2D图片，它可以用来贴图（添加物体的细节），
    // 纹理可以各种变形后贴到不同形状的区域内。
    // 这里直接用纹理显示视频帧

    GLint m_TextureUniformY;  // y纹理数据位置
    GLint m_TextureUniformUV;  // uv纹理数据位置

    GLuint m_TextureIdY;  // y纹理对象ID
    GLuint m_TextureIdUV;  // u纹理对象ID

    QOpenGLTexture* m_pTextureY;  // y纹理对象
    QOpenGLTexture* m_pTextureUV;  // u纹理对象

    // 着色器：控制GPU进行绘制
    QOpenGLShaderProgram* m_pShaderProgram;  // 着色器程序容器

    VideoDecoder *m_pVideoDecoder;

    int m_nVideoWidth;
    int m_nVideoHeight;

	std::thread *m_taskThread;

	std::atomic<bool> m_taskRunning;
	std::mutex m_taskMutex;
	std::condition_variable m_taskCondition;
};

