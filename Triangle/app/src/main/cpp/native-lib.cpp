#include <jni.h>
#include <android/native_window_jni.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <string>
#include <android/bitmap.h>

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

    // 绘制三角形
    glBindVertexArray(VAO);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    eglSwapBuffers(display, winSurface);

    // 清理资源
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
Java_com_example_triangle_YuvPlayer_drawTexture(JNIEnv *env, jobject thiz, jobject bitmap, jobject bitmap1,jobject surface) {

    // 顶点着色器
    const char *vertexSimpleShape = R"(#version 300 es
        layout (location = 0) in vec4 aPosition;  // 顶点坐标
        layout (location = 1) in vec2 aTexCoord;  // 纹理坐标
        out vec2 TexCoord;  // 传递到片段着色器的纹理坐标

        void main() {
            gl_Position = aPosition;
            TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);;
        }
    )";

    // 片段着色器
    const char *fragSimpleShape = R"(#version 300 es
        precision mediump float;
        in vec2 TexCoord;  // 从顶点着色器接收的纹理坐标
        out vec4 FragColor;  // 输出颜色
        uniform sampler2D ourTexture;  // 纹理采样器
        uniform sampler2D ourTexture1;  // 纹理采样器

        void main() {
            FragColor = mix(texture(ourTexture, TexCoord), texture(ourTexture1, TexCoord), 0.5);
        }
    )";

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // EGL 初始化
    ANativeWindow *nwin = ANativeWindow_fromSurface(env, surface);
    if (!nwin) {
        LOGD("ANativeWindow_fromSurface failed.");
        return;
    }
    LOGD("ANativeWindow_fromSurface succeeded.");

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGD("eglGetDisplay failed.");
        return;
    }
    LOGD("eglGetDisplay succeeded.");

    if (EGL_TRUE != eglInitialize(display, 0, 0)) {
        LOGD("eglInitialize failed.");
        return;
    }
    LOGD("eglInitialize succeeded.");

    if (EGL_TRUE != eglBindAPI(EGL_OPENGL_ES_API)) {
        LOGD("eglBindAPI failed.");
        return;
    }
    LOGD("eglBindAPI succeeded.");

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
        LOGD("eglChooseConfig failed.");
        return;
    }
    LOGD("eglChooseConfig succeeded.");

    EGLSurface winSurface = eglCreateWindowSurface(display, eglConfig, nwin, nullptr);
    if (winSurface == EGL_NO_SURFACE) {
        LOGD("eglCreateWindowSurface failed.");
        return;
    }
    LOGD("eglCreateWindowSurface succeeded.");

    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, eglConfig, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT) {
        LOGD("eglCreateContext failed.");
        return;
    }
    LOGD("eglCreateContext succeeded.");

    if (EGL_TRUE != eglMakeCurrent(display, winSurface, winSurface, context)) {
        LOGD("eglMakeCurrent failed.");
        return;
    }
    LOGD("eglMakeCurrent succeeded.");

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // 编译和链接着色器
    LOGD("Compiling shaders and linking program.");
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSimpleShape, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, log);
        LOGD("Vertex shader compilation failed: %s", log);
        return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragSimpleShape, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, log);
        LOGD("Fragment shader compilation failed: %s", log);
        return;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        LOGD("Shader program linking failed: %s", log);
        return;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // 设置顶点数组和纹理
    LOGD("Setting up vertex data and texture.");

    float vertices[] = {
            0.5f,  0.5f,  0.0f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.0f,  0.0f, 1.0f
    };
    unsigned int indices[] = {0, 1, 3, 1, 2, 3};
    GLuint VBO, VAO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    AndroidBitmapInfo bmpInfo,bmpInfo1;
    if (AndroidBitmap_getInfo(env, bitmap, &bmpInfo) < 0) {
        LOGD("AndroidBitmap_getInfo failed!");
        return;
    }
    if (AndroidBitmap_getInfo(env, bitmap1, &bmpInfo1) < 0) {
        LOGD("AndroidBitmap_getInfo failed!");
        return;
    }
    LOGD("Bitmap Info: width=%d, height=%d, format=%d", bmpInfo.width, bmpInfo.height, bmpInfo.format);
    LOGD("Bitmap Info: width=%d, height=%d, format=%d", bmpInfo1.width, bmpInfo1.height, bmpInfo1.format);
    void *bmpPixels,*bmpPixels1;
    if (AndroidBitmap_lockPixels(env, bitmap, &bmpPixels) < 0||AndroidBitmap_lockPixels(env, bitmap1, &bmpPixels1) < 0) {
        LOGD("AndroidBitmap_lockPixels failed!");
        return;
    }

    GLuint texture,texture1;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmpInfo.width, bmpInfo.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmpPixels);
    AndroidBitmap_unlockPixels(env, bitmap);

    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmpInfo1.width, bmpInfo1.height, 0, GL_RGBA,GL_UNSIGNED_BYTE, bmpPixels1);
    AndroidBitmap_unlockPixels(env, bitmap1);




    ///////////////////////////////////////////////////////////////////////////////////////////////
    // 渲染
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture1"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    eglSwapBuffers(display, winSurface);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // 清理资源

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &texture1);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, winSurface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    ANativeWindow_release(nwin);

    LOGD("drawTexture: Completed successfully.");
}









