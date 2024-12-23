#include <jni.h>
#include <android/native_window_jni.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <string>
#include <android/bitmap.h>
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "YuvPlayer", __VA_ARGS__))
// 初始化着色器函数
GLint initShader(const char *code, GLenum type) {
    GLint shader = glCreateShader(type);
    if (shader == 0) {
        LOGD("glCreateShader failed");
        return 0;
    }
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        LOGD("glCompileShader failed: %s", log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_triangle_YuvPlayer_drawTriangle(JNIEnv *env, jobject thiz, jobject surface) {
// 顶点着色器代码
const char *vertexSimpleShape = R"(#version 300 es
layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aColor;
out vec4 vTextColor;

void main() {
    gl_Position = aPosition;
    vTextColor = aColor;
}
)";
// 片段着色器代码
const char *fragSimpleShape = R"(#version 300 es
precision mediump float;
in vec4 vTextColor;
out vec4 fragColor;
void main() {
    fragColor = vTextColor;
}
)";
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // EGL 初始化
    ANativeWindow *nwin = ANativeWindow_fromSurface(env, surface);
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGD("eglGetDisplay failed");
        return;
    }
    if (EGL_TRUE != eglInitialize(display, 0, 0)) {
        LOGD("eglInitialize failed");
        return;
    }
    eglBindAPI(EGL_OPENGL_ES_API); // 确保绑定 OpenGL ES API
    EGLConfig eglConfig;
    EGLint configNum;
    EGLint configSpec[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 16,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };
    if (EGL_TRUE != eglChooseConfig(display, configSpec, &eglConfig, 1, &configNum)) {
        EGLint error = eglGetError();
        LOGD("eglChooseConfig failed: %x", error);
        return;
    }
    EGLSurface winSurface = eglCreateWindowSurface(display, eglConfig, nwin, nullptr);
    if (winSurface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        LOGD("eglCreateWindowSurface failed: %x", error);
        return;
    }
    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, eglConfig, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT) {
        EGLint error = eglGetError();
        LOGD("eglCreateContext failed: %x", error);
        return;
    }
    if (EGL_TRUE != eglMakeCurrent(display, winSurface, winSurface, context)) {
        EGLint error = eglGetError();
        LOGD("eglMakeCurrent failed: %x", error);
        return;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // 初始化着色器
    GLint vsh = initShader(vertexSimpleShape, GL_VERTEX_SHADER);
    GLint fsh = initShader(fragSimpleShape, GL_FRAGMENT_SHADER);
    if (vsh == 0 || fsh == 0) {
        LOGD("Shader initialization failed");
        return;
    }
    GLint program = glCreateProgram();
    glAttachShader(program, vsh);
    glAttachShader(program, fsh);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == 0) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        LOGD("glLinkProgram failed: %s", log);
        return;
    }
    LOGD("glLinkProgram success");
    glUseProgram(program);
    ///////////////////////////////////////////////////////////////////////
    // 定义顶点和颜色数据
    static float triangleVerWithColor[] = {
            0.0f, 0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f
    };

    // 配置 VAO 和 VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVerWithColor), triangleVerWithColor, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // 顶点位置
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // 顶点颜色
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 绘制立方体
    glBindVertexArray(VAO);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    eglSwapBuffers(display, winSurface);
    // 清理资源////////////////////////////////////////////////////////
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteShader(vsh);
    glDeleteShader(fsh);
    glDeleteProgram(program);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, winSurface);
    eglTerminate(display);
    LOGD("Rendering completed");
}











extern "C"
JNIEXPORT void JNICALL
Java_com_example_triangle_YuvPlayer_drawTexture(JNIEnv *env, jobject thiz, jobject bitmaps, jobject surface) {
    // 顶点着色器
    const char *vertexShaderSource = R"(#version 300 es
        layout (location = 0) in vec4 aPosition; // 顶点坐标
        layout (location = 1) in vec2 aTextCoord; // 纹理坐标
        out vec2 TexCoord; // 输出纹理坐标
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            TexCoord = vec2(aTextCoord.x, 1.0 - aTextCoord.y); // 翻转y坐标
            gl_Position = projection * view * model * aPosition;
        }
    )";

    // 片段着色器
    const char *fragShaderSource = R"(#version 300 es
        precision mediump float;
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D ourTexture;

        void main() {
            FragColor = texture(ourTexture, TexCoord);
        }
    )";

    // EGL 初始化
    ANativeWindow *nwin = ANativeWindow_fromSurface(env, surface);
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLConfig eglConfig;
    EGLint configSpec[] = {
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE
    };
    EGLint numConfigs;
    if (eglChooseConfig(display, configSpec, &eglConfig, 1, &numConfigs) == EGL_FALSE) {
        // 错误处理
        return;
    }

    EGLSurface winSurface = eglCreateWindowSurface(display, eglConfig, nwin, nullptr);
    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, eglConfig, EGL_NO_CONTEXT, ctxAttr);
    if (eglMakeCurrent(display, winSurface, winSurface, context) == EGL_FALSE) {
        // 错误处理
        return;
    }

    glEnable(GL_DEPTH_TEST); // 启用深度测试

    // 编译着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glUseProgram(program);



    float vertices[] = {
            // 顶点坐标          纹理坐标
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,//0后↙
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,//1后↘
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,//2后↗
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,//3后↖
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,//4前↙
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,//5前↘
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,//6前↗
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,//7前↖
    };
    unsigned int indices[] = {
            //背面
            0, 3, 1, // first triangle
            3, 2, 1, // second triangle
            //上面
            2, 3, 7, // first triangle
            7, 6, 2,  // second triangle
            //左面
            3, 0, 4, // first triangle
            4, 7, 3, // second triangle
            //右面
            5, 1, 2, // first triangle
            2, 6, 5, // second triangle
            //下面
            4, 0, 1, // first triangle
            1, 5, 4,// second triangle
            //前面
            4, 5, 6, // first triangle
            6, 7, 4, // second triangle
    };


    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 加载纹理
    unsigned int textures[6];
    glGenTextures(6, textures);

    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID sizeMethod = env->GetMethodID(arrayListClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(arrayListClass, "get", "(I)Ljava/lang/Object;");
    jint ref_size = env->CallIntMethod(bitmaps, sizeMethod);

    for (int i = 0; i < ref_size; ++i) {
        jobject bitmap = env->CallObjectMethod(bitmaps, getMethod, i);
        AndroidBitmapInfo bmpInfo;
        if (AndroidBitmap_getInfo(env, bitmap, &bmpInfo) < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "YuvPlayer", "Error getting bitmap info for texture %d", i);
            continue;
        }

        void *pixels;
        if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "YuvPlayer", "Error locking pixels for texture %d", i);
            continue;
        }

        // 输出纹理信息
        __android_log_print(ANDROID_LOG_DEBUG, "YuvPlayer", "Loading texture %d: width = %d, height = %d", i, bmpInfo.width, bmpInfo.height);

        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmpInfo.width, bmpInfo.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

        AndroidBitmap_unlockPixels(env, bitmap);
    }

    // 预先计算视图和投影矩阵
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1080.0f / 2400.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    static int frameCount = 0;      // 帧计数器
    // 渲染循环
    while (true) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // 基于帧计数来计算旋转角度，保持原有方法
        frameCount+=1;  // 每渲染一帧，帧计数器加1
        float angle = (frameCount % 360);  // 旋转角度基于帧计数

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(-0.5f, -0.5f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // 绘制每个面，注意纹理绑定
        for (int i = 0; i < 6; ++i) {
            glBindTexture(GL_TEXTURE_2D, textures[i]);  // 绑定正确的纹理
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * sizeof(unsigned int) * 6));
        }

        eglSwapBuffers(display, winSurface);
    }
}








