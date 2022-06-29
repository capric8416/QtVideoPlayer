#include "YuvWidget.h"

#include <QMouseEvent>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>

#include "videodecoder.h"

extern "C"
{
	#include <libavformat/avformat.h>
}




QOpenGLYUVWidget::QOpenGLYUVWidget(QWidget* parent)
	: QOpenGLWidget(parent)

	, m_TextureUniformY(0)
	, m_TextureUniformUV(0)

	, m_TextureIdY(0)
	, m_TextureIdUV(0)

	, m_pTextureY(nullptr)
	, m_pTextureUV(nullptr)

	, m_pShaderProgram(nullptr)

	, m_pVideoDecoder(nullptr)

    , m_nVideoWidth(0)
    , m_nVideoHeight(0)

	, m_taskThread(nullptr)
{
	StartDecoder();

	StartTask();
}

QOpenGLYUVWidget::~QOpenGLYUVWidget()
{
	StopTask();

	StopDecoder();

    makeCurrent();

    if(m_pShaderProgram != nullptr) {
        m_pShaderProgram->release();
        delete m_pShaderProgram;
        m_pShaderProgram = nullptr;
    }

    if(m_pTextureY != nullptr) {
        m_pTextureY->release();
        delete m_pTextureY;
        m_pTextureY = nullptr;
    }

    if(m_pTextureUV != nullptr) {
        m_pTextureUV->release();
        delete m_pTextureUV;
        m_pTextureUV = nullptr;
    }
}


void QOpenGLYUVWidget::StartTask()
{
	m_taskRunning = true;

	m_taskThread = new std::thread([&]() {
		while (m_taskRunning) {
			std::unique_lock<std::mutex> locker(m_taskMutex);
			m_taskCondition.wait(locker);

			if (m_taskRunning) {
				// 解码一帧数据
				if (m_pVideoDecoder->DecodeOneFrame()) {
					// 刷新界面, 触发paintGL接口
					emit update();
				}
			}
		}
	});
}


void QOpenGLYUVWidget::StopTask()
{
	if (m_taskThread != nullptr)
	{
		m_taskRunning = false;

		m_taskCondition.notify_one();
		
		m_taskThread->join();

		delete m_taskThread;
		m_taskThread = nullptr;
	}
}


void QOpenGLYUVWidget::WakeTask()
{
	m_taskCondition.notify_one();
}


void QOpenGLYUVWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);

    // OpenGL渲染管线依赖着色器来处理传入的数据
    // 着色器就是使用OpenGL Shading Language编写的函数

    // 顶点着色器源码
    const char* vertexShaderSource = "                                \
			attribute vec4 vertexIn;                                  \
            attribute vec2 textureIn;                                 \
            varying vec2 textureOut;                                  \
            void main(void)                                           \
            {                                                         \
                gl_Position = vertexIn;                               \
                textureOut = textureIn;                               \
            }                                                         \
           ";

    // 片段着色器源码, nv12转rgb
    const char* fragmentShaderSource = "                              \
		    varying vec2 textureOut;                                  \
            uniform sampler2D tex_y;                                  \
            uniform sampler2D tex_uv;                                 \
            void main(void)                                           \
            {                                                         \
                vec3 yuv;                                             \
                vec3 rgb;                                             \
				yuv.x = texture2D(tex_y, textureOut).r - 0.0625;      \
                yuv.y = texture2D(tex_uv, textureOut).r - 0.5;        \
	            yuv.z = texture2D(tex_uv, textureOut).g - 0.5;        \
	            rgb = mat3(1,              1,       1,                \
		                   0,       -0.39465, 2.03211,                \
		                   1.13983, -0.58060,       0) * yuv;         \
                gl_FragColor = vec4(rgb, 1);                          \
            }";

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

    // 创建着色器程序容器
    m_pShaderProgram = new QOpenGLShaderProgram;
    // 将顶点着色器添加到程序容器
    m_pShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    // 将片段着色器添加到程序容器
    m_pShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    // 绑定属性vertexIn到指定位置ATTRIB_VERTEX, 该属性在顶点着色源码其中有声明
    m_pShaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
    // 绑定属性textureIn到指定位置ATTRIB_TEXTURE, 该属性在顶点着色源码其中有声明
    m_pShaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);
    // 链接所有所有添入到的着色器程序
    m_pShaderProgram->link();
    // 激活所有链接
    m_pShaderProgram->bind();

    // 读取着色器中的数据变量tex_y, tex_u, tex_v的位置, 这些变量的声明可以在片段着色器源码中可以看到
    m_TextureUniformY = m_pShaderProgram->uniformLocation("tex_y");
    m_TextureUniformUV = m_pShaderProgram->uniformLocation("tex_uv");

    // 顶点矩阵
    static const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
        1.0f,  -1.0f,
        -1.0f,  1.0f,
        1.0f,   1.0f,
    };

    // 纹理矩阵
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    // 设置属性ATTRIB_VERTEX的顶点矩阵值以及格式
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
    // 设置属性ATTRIB_TEXTURE的纹理矩阵值以及格式
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
    // 启用ATTRIB_VERTEX属性的数据, 默认是关闭的
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    // 启用ATTRIB_TEXTURE属性的数据, 默认是关闭的
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    // 分别创建y, uv纹理对象
    m_pTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureUV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureY->create();
    m_pTextureUV->create();

    // 获取返回y分量的纹理索引值
    m_TextureIdY = m_pTextureY->textureId();
    // 获取返回u分量的纹理索引值
    m_TextureIdUV = m_pTextureUV->textureId();

    // 设置背景色
    glClearColor(0.0, 0.0, 0.0, 1.0);
}


void QOpenGLYUVWidget::resizeGL(int w, int h)
{
    if (h == 0) // 防止被零除
    {
        h = 1;  // 将高设为1
    }

    // 设置视口
    glViewport(0, 0, w, h);
}


void QOpenGLYUVWidget::paintGL()
{
    AVFrame *pFrame = m_pVideoDecoder->GetOneFrame();

    // 加载y数据纹理
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureIdY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pFrame->linesize[0], pFrame->height, 0, GL_RED, GL_UNSIGNED_BYTE, pFrame->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// 指定y纹理要使用新值
	glUniform1i(m_TextureUniformY, 0);  // 0对应纹理单元GL_TEXTURE0

    // 加载uv数据纹理
    glActiveTexture(GL_TEXTURE1); // 激活纹理单元GL_TEXTURE1
    glBindTexture(GL_TEXTURE_2D, m_TextureIdUV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, pFrame->linesize[1] >> 1, pFrame->height >> 1, 0, GL_RG, GL_UNSIGNED_BYTE, pFrame->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// 指定uv纹理要使用新值
	glUniform1i(m_TextureUniformUV, 1);  // 1对应纹理单元GL_TEXTURE1

    // 使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_pVideoDecoder->ReleaseOneFrame();

    return;
}


void QOpenGLYUVWidget::StartDecoder()
{
	m_pVideoDecoder = new VideoDecoder();
	m_nVideoWidth = m_pVideoDecoder->GetVideoWidth();
	m_nVideoHeight = m_pVideoDecoder->GetVideoHeight();
}


void QOpenGLYUVWidget::StopDecoder()
{
	if (m_pVideoDecoder != nullptr)
	{
		delete m_pVideoDecoder;
		m_pVideoDecoder = nullptr;
	}
}