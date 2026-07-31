#include <vector>
#include <string>
#include <cstring>
#include <cassert>

#include <android/log.h>
#include <android_native_app_glue.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#ifndef XR_USE_PLATFORM_ANDROID
#define XR_USE_PLATFORM_ANDROID
#endif
#ifndef XR_USE_GRAPHICS_API_OPENGL_ES
#define XR_USE_GRAPHICS_API_OPENGL_ES
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#define LOG_TAG "TED"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

struct Swapchain {
    XrSwapchain handle{XR_NULL_HANDLE};
    int width{0};
    int height{0};
    GLuint depthRenderbuffer{0};
    std::vector<XrSwapchainImageOpenGLESKHR> images;
};

struct Engine {
    struct android_app* app{nullptr};

    EGLDisplay display{EGL_NO_DISPLAY};
    EGLContext context{EGL_NO_CONTEXT};
    EGLSurface surface{EGL_NO_SURFACE};
    EGLConfig config{nullptr};

    bool windowInitialized{false};
    bool sessionRunning{false};

    XrInstance instance{XR_NULL_HANDLE};
    XrSystemId systemId{XR_NULL_SYSTEM_ID};
    XrSession session{XR_NULL_HANDLE};
    XrSessionState sessionState{XR_SESSION_STATE_UNKNOWN};
    XrSpace appSpace{XR_NULL_HANDLE};

    XrViewConfigurationType viewConfigType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
    std::vector<XrViewConfigurationView> configViews;
    std::vector<Swapchain> swapchains;
    std::vector<XrView> views;

    GLuint framebuffer{0};

    // Extension function pointers
    PFN_xrInitializeLoaderKHR pfnInitializeLoaderKHR{nullptr};
    PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR{nullptr};
};

static bool initEGL(Engine* engine) {
    engine->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (engine->display == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }

    EGLint major = 0, minor = 0;
    if (!eglInitialize(engine->display, &major, &minor)) {
        LOGE("eglInitialize failed");
        return false;
    }
    LOGI("EGL Initialized: %d.%d", major, minor);

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    EGLint numConfigs = 0;
    if (!eglChooseConfig(engine->display, attribs, &engine->config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("eglChooseConfig failed");
        return false;
    }

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    engine->context = eglCreateContext(engine->display, engine->config, EGL_NO_CONTEXT, contextAttribs);
    if (engine->context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return false;
    }

    engine->surface = eglCreateWindowSurface(engine->display, engine->config, engine->app->window, nullptr);
    if (engine->surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return false;
    }

    if (!eglMakeCurrent(engine->display, engine->surface, engine->surface, engine->context)) {
        LOGE("eglMakeCurrent failed");
        return false;
    }

    LOGI("EGL surface and context bound successfully.");
    return true;
}

static void terminateEGL(Engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
            engine->surface = EGL_NO_SURFACE;
        }
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
            engine->context = EGL_NO_CONTEXT;
        }
        eglTerminate(engine->display);
        engine->display = EGL_NO_DISPLAY;
    }
    LOGI("EGL terminated.");
}

static bool initOpenXR(Engine* engine) {
    // 1. Initialize OpenXR Loader for Android
    xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)&engine->pfnInitializeLoaderKHR);
    if (engine->pfnInitializeLoaderKHR) {
        XrLoaderInitInfoAndroidKHR loaderInitInfo{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        loaderInitInfo.applicationVM = engine->app->activity->vm;
        loaderInitInfo.applicationContext = engine->app->activity->clazz;
        XrResult res = engine->pfnInitializeLoaderKHR((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfo);
        if (XR_FAILED(res)) {
            LOGE("xrInitializeLoaderKHR failed: %d", res);
            return false;
        }
    } else {
        LOGI("xrInitializeLoaderKHR symbol not present; proceeding with standard create info.");
    }

    // 2. Enumerate & Verify Instance Extensions
    uint32_t extCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
    std::vector<XrExtensionProperties> extensionProperties(extCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, extensionProperties.data());

    const char* requestedExtensions[] = {
        XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
        XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME
    };

    std::vector<const char*> enabledExtensions;
    for (const char* reqExt : requestedExtensions) {
        bool found = false;
        for (const auto& extProp : extensionProperties) {
            if (strcmp(reqExt, extProp.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (found) {
            enabledExtensions.push_back(reqExt);
            LOGI("Enabled OpenXR extension: %s", reqExt);
        } else {
            LOGE("Required OpenXR extension NOT supported by runtime: %s", reqExt);
            return false;
        }
    }

    // 3. Create Instance
    XrInstanceCreateInfoAndroidKHR createInfoAndroid{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    createInfoAndroid.applicationVM = engine->app->activity->vm;
    createInfoAndroid.applicationActivity = engine->app->activity->clazz;

    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.next = &createInfoAndroid;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.enabledExtensionNames = enabledExtensions.data();
    strncpy(createInfo.applicationInfo.applicationName, "TED OpenXR Native", XR_MAX_APPLICATION_NAME_SIZE);
    createInfo.applicationInfo.applicationVersion = 1;
    strncpy(createInfo.applicationInfo.engineName, "TED Engine", XR_MAX_ENGINE_NAME_SIZE);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrResult res = xrCreateInstance(&createInfo, &engine->instance);
    if (XR_FAILED(res)) {
        LOGE("xrCreateInstance failed: %d", res);
        return false;
    }
    LOGI("OpenXR Instance created successfully.");

    // 4. Get Proc Addrs
    xrGetInstanceProcAddr(engine->instance, "xrGetOpenGLESGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&engine->pfnGetOpenGLESGraphicsRequirementsKHR);

    // 5. Get System
    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    res = xrGetSystem(engine->instance, &systemInfo, &engine->systemId);
    if (XR_FAILED(res)) {
        LOGE("xrGetSystem failed: %d", res);
        return false;
    }
    LOGI("OpenXR System ID obtained: %llu", (unsigned long long)engine->systemId);

    // 6. Query GLES requirements
    if (engine->pfnGetOpenGLESGraphicsRequirementsKHR) {
        XrGraphicsRequirementsOpenGLESKHR graphicsReq{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        res = engine->pfnGetOpenGLESGraphicsRequirementsKHR(engine->instance, engine->systemId, &graphicsReq);
        if (XR_FAILED(res)) {
            LOGE("xrGetOpenGLESGraphicsRequirementsKHR failed: %d", res);
            return false;
        }
        LOGI("GLES Graphics Requirements satisfied. Minimum API: %u.%u",
             XR_VERSION_MAJOR(graphicsReq.minApiVersionSupported),
             XR_VERSION_MINOR(graphicsReq.minApiVersionSupported));
    }

    // 7. Create Session
    XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
    graphicsBinding.display = engine->display;
    graphicsBinding.config = engine->config;
    graphicsBinding.context = engine->context;

    XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionCreateInfo.next = &graphicsBinding;
    sessionCreateInfo.systemId = engine->systemId;
    res = xrCreateSession(engine->instance, &sessionCreateInfo, &engine->session);
    if (XR_FAILED(res)) {
        LOGE("xrCreateSession failed: %d", res);
        return false;
    }
    LOGI("OpenXR Session created successfully.");

    // 8. Create Space (STAGE / LOCAL)
    XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
    res = xrCreateReferenceSpace(engine->session, &spaceCreateInfo, &engine->appSpace);
    if (XR_FAILED(res)) {
        LOGW("STAGE space failed; falling back to LOCAL space...");
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        res = xrCreateReferenceSpace(engine->session, &spaceCreateInfo, &engine->appSpace);
        if (XR_FAILED(res)) {
            LOGE("xrCreateReferenceSpace LOCAL failed: %d", res);
            return false;
        }
    }
    LOGI("OpenXR Reference Space created.");

    // 9. Create Swapchains & Depth Renderbuffers
    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(engine->instance, engine->systemId, engine->viewConfigType, 0, &viewCount, nullptr);
    engine->configViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    xrEnumerateViewConfigurationViews(engine->instance, engine->systemId, engine->viewConfigType, viewCount, &viewCount, engine->configViews.data());
    engine->views.resize(viewCount, {XR_TYPE_VIEW});

    engine->swapchains.resize(viewCount);
    for (uint32_t i = 0; i < viewCount; ++i) {
        XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        swapchainCreateInfo.format = GL_RGBA8;
        swapchainCreateInfo.sampleCount = 1;
        swapchainCreateInfo.width = engine->configViews[i].recommendedImageRectWidth;
        swapchainCreateInfo.height = engine->configViews[i].recommendedImageRectHeight;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.mipCount = 1;

        engine->swapchains[i].width = swapchainCreateInfo.width;
        engine->swapchains[i].height = swapchainCreateInfo.height;

        res = xrCreateSwapchain(engine->session, &swapchainCreateInfo, &engine->swapchains[i].handle);
        if (XR_FAILED(res)) {
            LOGE("xrCreateSwapchain[%d] failed: %d", i, res);
            return false;
        }

        uint32_t imgCount = 0;
        xrEnumerateSwapchainImages(engine->swapchains[i].handle, 0, &imgCount, nullptr);
        engine->swapchains[i].images.resize(imgCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
        xrEnumerateSwapchainImages(engine->swapchains[i].handle, imgCount, &imgCount, (XrSwapchainImageBaseHeader*)engine->swapchains[i].images.data());

        // Create 24-bit Depth Renderbuffer matching swapchain dimensions
        glGenRenderbuffers(1, &engine->swapchains[i].depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, engine->swapchains[i].depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, engine->swapchains[i].width, engine->swapchains[i].height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        LOGI("Swapchain[%d] created (%dx%d), image count: %u, depth renderbuffer ID: %u",
             i, engine->swapchains[i].width, engine->swapchains[i].height, imgCount, engine->swapchains[i].depthRenderbuffer);
    }

    // 10. Framebuffer initialization
    glGenFramebuffers(1, &engine->framebuffer);

    return true;
}

static void terminateOpenXR(Engine* engine) {
    if (engine->framebuffer) {
        glDeleteFramebuffers(1, &engine->framebuffer);
        engine->framebuffer = 0;
    }
    for (auto& sc : engine->swapchains) {
        if (sc.depthRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &sc.depthRenderbuffer);
            sc.depthRenderbuffer = 0;
        }
        if (sc.handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(sc.handle);
            sc.handle = XR_NULL_HANDLE;
        }
    }
    engine->swapchains.clear();
    if (engine->appSpace != XR_NULL_HANDLE) {
        xrDestroySpace(engine->appSpace);
        engine->appSpace = XR_NULL_HANDLE;
    }
    if (engine->session != XR_NULL_HANDLE) {
        xrDestroySession(engine->session);
        engine->session = XR_NULL_HANDLE;
    }
    if (engine->instance != XR_NULL_HANDLE) {
        xrDestroyInstance(engine->instance);
        engine->instance = XR_NULL_HANDLE;
    }
    LOGI("OpenXR resources destroyed.");
}

static void pollOpenXREvents(Engine* engine) {
    XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(engine->instance, &eventData) == XR_SUCCESS) {
        if (eventData.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            auto stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData);
            LOGI("Session state changed from %d to %d", engine->sessionState, stateEvent->state);
            engine->sessionState = stateEvent->state;

            if (engine->sessionState == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                beginInfo.primaryViewConfigurationType = engine->viewConfigType;
                XrResult res = xrBeginSession(engine->session, &beginInfo);
                if (XR_SUCCEEDED(res)) {
                    engine->sessionRunning = true;
                    LOGI("xrBeginSession succeeded.");
                } else {
                    LOGE("xrBeginSession failed: %d", res);
                }
            } else if (engine->sessionState == XR_SESSION_STATE_STOPPING) {
                engine->sessionRunning = false;
                xrEndSession(engine->session);
                LOGI("xrEndSession completed.");
            } else if (engine->sessionState == XR_SESSION_STATE_EXITING || engine->sessionState == XR_SESSION_STATE_LOSS_PENDING) {
                engine->sessionRunning = false;
                if (engine->app && engine->app->activity) {
                    LOGI("Requesting ANativeActivity_finish due to session exit/loss.");
                    ANativeActivity_finish(engine->app->activity);
                }
            }
        }
        eventData = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

static void renderFrame(Engine* engine) {
    if (!engine->sessionRunning) return;

    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    XrResult res = xrWaitFrame(engine->session, &waitInfo, &frameState);
    if (XR_FAILED(res)) {
        LOGE("xrWaitFrame failed: %d", res);
        return;
    }

    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    res = xrBeginFrame(engine->session, &beginInfo);
    if (XR_FAILED(res)) {
        LOGE("xrBeginFrame failed: %d", res);
        return;
    }

    std::vector<XrCompositionLayerBaseHeader*> layers;
    XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    std::vector<XrCompositionLayerProjectionView> projectionViews;

    if (frameState.shouldRender) {
        XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        locateInfo.viewConfigurationType = engine->viewConfigType;
        locateInfo.displayTime = frameState.predictedDisplayTime;
        locateInfo.space = engine->appSpace;

        XrViewState viewState{XR_TYPE_VIEW_STATE};
        uint32_t viewCount = (uint32_t)engine->views.size();
        xrLocateViews(engine->session, &locateInfo, &viewState, viewCount, &viewCount, engine->views.data());

        projectionViews.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});

        for (uint32_t i = 0; i < viewCount; ++i) {
            uint32_t imgIndex = 0;
            XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            xrAcquireSwapchainImage(engine->swapchains[i].handle, &acquireInfo, &imgIndex);

            XrSwapchainImageWaitInfo waitImageInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitImageInfo.timeout = XR_INFINITE_DURATION;
            xrWaitSwapchainImage(engine->swapchains[i].handle, &waitImageInfo);

            projectionViews[i].pose = engine->views[i].pose;
            projectionViews[i].fov = engine->views[i].fov;
            projectionViews[i].subImage.swapchain = engine->swapchains[i].handle;
            projectionViews[i].subImage.imageRect.offset = {0, 0};
            projectionViews[i].subImage.imageRect.extent = {engine->swapchains[i].width, engine->swapchains[i].height};

            // Render to Framebuffer with Color & Depth Attachments
            glBindFramebuffer(GL_FRAMEBUFFER, engine->framebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, engine->swapchains[i].images[imgIndex].image, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, engine->swapchains[i].depthRenderbuffer);

            glViewport(0, 0, engine->swapchains[i].width, engine->swapchains[i].height);

            // Eye color distinction: Left eye cyan (0.1, 0.5, 0.8), Right eye magenta (0.8, 0.2, 0.6)
            if (i == 0) {
                glClearColor(0.1f, 0.5f, 0.8f, 1.0f);
            } else {
                glClearColor(0.8f, 0.2f, 0.6f, 1.0f);
            }
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(engine->swapchains[i].handle, &releaseInfo);
        }

        projectionLayer.space = engine->appSpace;
        projectionLayer.viewCount = (uint32_t)projectionViews.size();
        projectionLayer.views = projectionViews.data();
        layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&projectionLayer));
    }

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = (uint32_t)layers.size();
    endInfo.layers = layers.data();
    xrEndFrame(engine->session, &endInfo);
}

static void handleAppCmd(struct android_app* app, int32_t cmd) {
    Engine* engine = reinterpret_cast<Engine*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW received");
            if (app->window != nullptr) {
                if (initEGL(engine)) {
                    if (initOpenXR(engine)) {
                        engine->windowInitialized = true;
                    }
                }
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW received");
            engine->windowInitialized = false;
            terminateOpenXR(engine);
            terminateEGL(engine);
            break;
        case APP_CMD_DESTROY:
            LOGI("APP_CMD_DESTROY received");
            break;
        default:
            break;
    }
}

void android_main(struct android_app* app) {
    Engine engine{};
    engine.app = app;
    app->userData = &engine;
    app->onAppCmd = handleAppCmd;

    LOGI("TED OpenXR Native app started via android_main");

    while (app->destroyRequested == 0) {
        int events = 0;
        struct android_poll_source* source = nullptr;

        while (ALooper_pollOnce(engine.windowInitialized && engine.sessionRunning ? 0 : -1,
                                nullptr, &events, (void**)&source) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            if (app->destroyRequested != 0) {
                break;
            }
        }

        if (engine.instance != XR_NULL_HANDLE) {
            pollOpenXREvents(&engine);
        }

        if (engine.windowInitialized && engine.sessionRunning) {
            renderFrame(&engine);
        }
    }

    terminateOpenXR(&engine);
    terminateEGL(&engine);
    LOGI("TED OpenXR Native app exited successfully.");
}
