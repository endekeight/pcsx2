#include "GLContextEGLAndroid.h"

GLContextEGLAndroid::GLContextEGLAndroid(const WindowInfo &wi)
        : GLContextEGL(wi)
{
}
GLContextEGLAndroid::~GLContextEGLAndroid() = default;

std::unique_ptr<GLContext> GLContextEGLAndroid::Create(const WindowInfo &wi, std::span<const Version> versions_to_try, Error* error) {
    std::unique_ptr<GLContextEGLAndroid> context = std::make_unique<GLContextEGLAndroid>(wi);
    if (!context->Initialize(versions_to_try, error))
    {
        return nullptr;
    }
    return context;
}

std::unique_ptr<GLContext> GLContextEGLAndroid::CreateSharedContext(const WindowInfo &wi, Error* error) {
    std::unique_ptr<GLContextEGLAndroid> context = std::make_unique<GLContextEGLAndroid>(wi);
    context->m_display = m_display;
    if (!context->CreateContextAndSurface(m_version, m_context, false))
        return nullptr;

    return context;
}

void GLContextEGLAndroid::ResizeSurface(u32 new_surface_width, u32 new_surface_height) {
    GLContextEGL::ResizeSurface(new_surface_width, new_surface_height);
}

EGLSurface GLContextEGLAndroid::CreatePlatformSurface(EGLConfig config, void* win, Error* error) {
    return {};
}

EGLDisplay GLContextEGLAndroid::GetPlatformDisplay(Error* error)
{
	EGLDisplay dpy = GetFallbackDisplay(error);
	return dpy;
}