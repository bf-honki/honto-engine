#include "honto/Application.h"

#include "honto/Input.h"
#include "honto/SceneGraph.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace honto
{
    AppConfig Application::SanitizeConfig(AppConfig config)
    {
        config.windowWidth = std::max(320, config.windowWidth);
        config.windowHeight = std::max(180, config.windowHeight);
        config.renderWidth = std::max(1, config.renderWidth);
        config.renderHeight = std::max(1, config.renderHeight);
        config.opacity = std::clamp(config.opacity, 0.1f, 1.0f);
        if (config.title.empty())
        {
            config.title = "HonTo Window";
        }
        if (config.windowId.empty())
        {
            config.windowId = config.title;
        }

        return config;
    }

    int Application::Run(Game& game)
    {
        Director::Get().AttachApplication(this);
        InitializeContext(m_MainContext, game.GetConfig(), game.CreateInitialScene(), true);

        for (WindowStartup& windowStartup : game.CreateAdditionalWindows())
        {
            m_AdditionalContexts.emplace_back();
            InitializeContext(
                m_AdditionalContexts.back(),
                windowStartup.config,
                windowStartup.createScene ? windowStartup.createScene() : nullptr,
                windowStartup.closeStopsGame
            );
        }

        using clock = std::chrono::high_resolution_clock;
        auto previousTime = clock::now();

        while (m_MainContext.window != nullptr && m_MainContext.window->IsOpen())
        {
            Window::PumpMessages();

            if (!m_PendingWindows.empty())
            {
                for (WindowStartup& startup : m_PendingWindows)
                {
                    WindowContext context;
                    InitializeContext(
                        context,
                        startup.config,
                        startup.createScene ? startup.createScene() : nullptr,
                        startup.closeStopsGame
                    );
                    if (startup.focusWindow && context.window != nullptr)
                    {
                        context.window->Focus();
                    }
                    m_AdditionalContexts.push_back(std::move(context));
                }

                m_PendingWindows.clear();
            }

            bool shouldExit = false;
            for (WindowContext& context : m_AdditionalContexts)
            {
                if (context.closeStopsGame && context.window != nullptr && !context.window->IsOpen())
                {
                    shouldExit = true;
                    break;
                }
            }

            if (shouldExit)
            {
                break;
            }

            const auto currentTime = clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;
            deltaTime = std::min(deltaTime, 0.1f);

            UpdateContext(m_MainContext, deltaTime);
            for (WindowContext& context : m_AdditionalContexts)
            {
                UpdateContext(context, deltaTime);
            }

            RenderContext(m_MainContext);
            for (WindowContext& context : m_AdditionalContexts)
            {
                RenderContext(context);
            }
        }

        for (WindowContext& context : m_AdditionalContexts)
        {
            ShutdownContext(context);
        }
        ShutdownContext(m_MainContext);
        Director::Get().DetachApplication(this);

        return 0;
    }

    void Application::SetScene(std::unique_ptr<Scene> scene, const SceneTransition& transition)
    {
        RequestSceneChange(ActiveContext(), std::move(scene), transition);
    }

    Scene* Application::ActiveScene()
    {
        return ActiveContext().scene.get();
    }

    const Scene* Application::ActiveScene() const
    {
        return ActiveContext().scene.get();
    }

    bool Application::SetSceneForWindow(const std::string& windowIdOrTitle, std::unique_ptr<Scene> scene, const SceneTransition& transition, bool focusWindow)
    {
        WindowContext* context = FindContext(windowIdOrTitle);
        if (context == nullptr || scene == nullptr)
        {
            return false;
        }

        RequestSceneChange(*context, std::move(scene), transition);
        if (focusWindow && context->window != nullptr)
        {
            context->window->Focus();
        }

        return true;
    }

    bool Application::FocusWindow(const std::string& windowIdOrTitle)
    {
        WindowContext* context = FindContext(windowIdOrTitle);
        if (context == nullptr || context->window == nullptr)
        {
            return false;
        }

        context->window->Focus();
        return true;
    }

    bool Application::OpenWindow(WindowStartup startup, bool focusWindow)
    {
        startup.config = SanitizeConfig(std::move(startup.config));
        startup.focusWindow = focusWindow;

        if (WindowContext* existing = FindContext(startup.config.windowId))
        {
            if (startup.createScene)
            {
                RequestSceneChange(*existing, startup.createScene(), {});
            }

            if (focusWindow && existing->window != nullptr)
            {
                existing->window->Focus();
            }

            return true;
        }

        if (WindowContext* existing = FindContext(startup.config.title))
        {
            if (startup.createScene)
            {
                RequestSceneChange(*existing, startup.createScene(), {});
            }

            if (focusWindow && existing->window != nullptr)
            {
                existing->window->Focus();
            }

            return true;
        }

        m_PendingWindows.push_back(std::move(startup));
        return true;
    }

    bool Application::CloseWindow(const std::string& windowIdOrTitle)
    {
        if (windowIdOrTitle.empty())
        {
            return false;
        }

        if (m_MainContext.window != nullptr &&
            (m_MainContext.config.windowId == windowIdOrTitle || m_MainContext.config.title == windowIdOrTitle))
        {
            m_MainContext.window->Close();
            return true;
        }

        for (WindowContext& context : m_AdditionalContexts)
        {
            if (context.window != nullptr &&
                (context.config.windowId == windowIdOrTitle || context.config.title == windowIdOrTitle))
            {
                context.window->Close();
                return true;
            }
        }

        return false;
    }

    Window* Application::FindWindow(const std::string& windowIdOrTitle)
    {
        WindowContext* context = FindContext(windowIdOrTitle);
        return context != nullptr ? context->window.get() : nullptr;
    }

    const Window* Application::FindWindow(const std::string& windowIdOrTitle) const
    {
        const WindowContext* context = FindContext(windowIdOrTitle);
        return context != nullptr ? context->window.get() : nullptr;
    }

    Application::WindowContext& Application::ActiveContext()
    {
        return (m_CurrentContext != nullptr) ? *m_CurrentContext : m_MainContext;
    }

    const Application::WindowContext& Application::ActiveContext() const
    {
        return (m_CurrentContext != nullptr) ? *m_CurrentContext : m_MainContext;
    }

    Application::WindowContext* Application::FindContext(const std::string& windowIdOrTitle)
    {
        if (m_MainContext.window != nullptr &&
            (m_MainContext.config.windowId == windowIdOrTitle || m_MainContext.config.title == windowIdOrTitle))
        {
            return &m_MainContext;
        }

        for (WindowContext& context : m_AdditionalContexts)
        {
            if (context.window != nullptr &&
                (context.config.windowId == windowIdOrTitle || context.config.title == windowIdOrTitle))
            {
                return &context;
            }
        }

        return nullptr;
    }

    const Application::WindowContext* Application::FindContext(const std::string& windowIdOrTitle) const
    {
        if (m_MainContext.window != nullptr &&
            (m_MainContext.config.windowId == windowIdOrTitle || m_MainContext.config.title == windowIdOrTitle))
        {
            return &m_MainContext;
        }

        for (const WindowContext& context : m_AdditionalContexts)
        {
            if (context.window != nullptr &&
                (context.config.windowId == windowIdOrTitle || context.config.title == windowIdOrTitle))
            {
                return &context;
            }
        }

        return nullptr;
    }

    void Application::InitializeContext(WindowContext& context, AppConfig config, std::unique_ptr<Scene> scene, bool closeStopsGame)
    {
        context.config = SanitizeConfig(std::move(config));
        context.closeStopsGame = closeStopsGame;
        context.window = std::make_unique<Window>(
            context.config.title,
            context.config.windowWidth,
            context.config.windowHeight,
            context.config.resizable,
            context.config.borderless,
            context.config.opacity,
            context.config.alwaysOnTop
        );
        context.backBuffer.resize(static_cast<std::size_t>(context.config.renderWidth * context.config.renderHeight));
        context.renderer.Attach(context.backBuffer.data(), context.config.renderWidth, context.config.renderHeight);
        context.renderer.ResetCamera();
        context.pendingScene.reset();
        context.activeTransition = {};
        context.transitionElapsed = 0.0f;
        context.transitionSwapped = false;
        LoadScene(context, std::move(scene));
    }

    void Application::ShutdownContext(WindowContext& context)
    {
        if (context.scene != nullptr)
        {
            m_CurrentContext = &context;
            context.scene->OnDestroy();
            m_CurrentContext = nullptr;
            context.scene.reset();
        }

        context.pendingScene.reset();
        context.window.reset();
        context.backBuffer.clear();
    }

    void Application::RequestSceneChange(WindowContext& context, std::unique_ptr<Scene> scene, const SceneTransition& transition)
    {
        if (scene == nullptr)
        {
            return;
        }

        context.pendingScene = std::move(scene);
        context.activeTransition = transition;
        context.transitionElapsed = 0.0f;
        context.transitionSwapped = false;

        if (context.scene == nullptr || transition.type == SceneTransition::Type::None || transition.duration <= 0.0f)
        {
            context.activeTransition = {};
        }
    }

    void Application::LoadScene(WindowContext& context, std::unique_ptr<Scene> scene)
    {
        if (context.scene != nullptr)
        {
            m_CurrentContext = &context;
            context.scene->OnDestroy();
            m_CurrentContext = nullptr;
        }

        context.scene = std::move(scene);
        context.renderer.ResetCamera();

        if (context.scene != nullptr)
        {
            m_CurrentContext = &context;
            context.scene->OnCreate();
            m_CurrentContext = nullptr;
        }
    }

    void Application::UpdateContext(WindowContext& context, float deltaTime)
    {
        if (context.window == nullptr || !context.window->IsOpen() || !context.window->IsVisible())
        {
            return;
        }

        m_CurrentContext = &context;
        Input::UpdateForWindow(*context.window, context.config.renderWidth, context.config.renderHeight);
        if (context.scene != nullptr)
        {
            context.scene->OnUpdate(deltaTime);
        }

        if (context.pendingScene != nullptr && context.activeTransition.type == SceneTransition::Type::None)
        {
            LoadScene(context, std::move(context.pendingScene));
            context.transitionElapsed = 0.0f;
            context.transitionSwapped = false;
        }
        else if (context.activeTransition.type != SceneTransition::Type::None)
        {
            const float halfDuration = std::max(0.001f, context.activeTransition.duration * 0.5f);
            context.transitionElapsed += deltaTime;

            if (!context.transitionSwapped && context.pendingScene != nullptr && context.transitionElapsed >= halfDuration)
            {
                LoadScene(context, std::move(context.pendingScene));
                context.transitionSwapped = true;
            }

            if (context.transitionElapsed >= halfDuration * 2.0f)
            {
                context.activeTransition = {};
                context.transitionElapsed = 0.0f;
                context.transitionSwapped = false;
                context.pendingScene.reset();
            }
        }

        m_CurrentContext = nullptr;
    }

    void Application::RenderContext(WindowContext& context)
    {
        if (context.window == nullptr || !context.window->IsOpen() || !context.window->IsVisible())
        {
            return;
        }

        context.renderer.BeginFrame(context.config.clearColor);

        m_CurrentContext = &context;
        if (context.scene != nullptr)
        {
            context.scene->OnRender(context.renderer);
        }

        if (context.activeTransition.type == SceneTransition::Type::Fade)
        {
            const float halfDuration = std::max(0.001f, context.activeTransition.duration * 0.5f);
            float alpha = 0.0f;

            if (context.transitionElapsed < halfDuration)
            {
                alpha = context.transitionElapsed / halfDuration;
            }
            else
            {
                alpha = 1.0f - ((context.transitionElapsed - halfDuration) / halfDuration);
            }

            alpha = std::clamp(alpha, 0.0f, 1.0f);
            Color overlay = context.activeTransition.color;
            overlay.a = static_cast<std::uint8_t>(static_cast<float>(overlay.a) * alpha);
            context.renderer.DrawFilledRect(
                {},
                { static_cast<float>(context.config.renderWidth), static_cast<float>(context.config.renderHeight) },
                overlay
            );
        }
        m_CurrentContext = nullptr;

        context.window->Present(context.backBuffer.data(), context.config.renderWidth, context.config.renderHeight);
    }
}
