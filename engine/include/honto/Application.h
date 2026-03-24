#pragma once

#include "Color.h"
#include "Renderer2D.h"
#include "Scene.h"
#include "Window.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace honto
{
    struct AppConfig
    {
        std::string title = "HonTo Engine";
        int windowWidth = 1280;
        int windowHeight = 720;
        int renderWidth = 320;
        int renderHeight = 180;
        Color clearColor { 16, 18, 28, 255 };
    };

    struct SceneTransition
    {
        enum class Type
        {
            None,
            Fade
        };

        Type type = Type::None;
        float duration = 0.0f;
        Color color { 0, 0, 0, 255 };
    };

    struct WindowStartup
    {
        AppConfig config {};
        bool closeStopsGame = false;
        std::function<std::unique_ptr<Scene>()> createScene;
    };

    class Game
    {
    public:
        virtual ~Game() = default;

        virtual AppConfig GetConfig() const
        {
            return {};
        }

        virtual std::unique_ptr<Scene> CreateInitialScene() = 0;

        virtual std::vector<WindowStartup> CreateAdditionalWindows()
        {
            return {};
        }
    };

    class Application
    {
    public:
        int Run(Game& game);

        void SetScene(std::unique_ptr<Scene> scene, const SceneTransition& transition = {});
        Scene* ActiveScene();
        const Scene* ActiveScene() const;

        const AppConfig& Config() const
        {
            return ActiveContext().config;
        }

        Renderer2D& GetRenderer()
        {
            return ActiveContext().renderer;
        }

        Window& GetWindow()
        {
            return *ActiveContext().window;
        }

    private:
        struct WindowContext
        {
            AppConfig config {};
            std::unique_ptr<Window> window;
            Renderer2D renderer;
            std::vector<std::uint32_t> backBuffer;
            std::unique_ptr<Scene> scene;
            std::unique_ptr<Scene> pendingScene;
            SceneTransition activeTransition {};
            float transitionElapsed = 0.0f;
            bool transitionSwapped = false;
            bool closeStopsGame = false;
        };

        static AppConfig SanitizeConfig(AppConfig config);
        WindowContext& ActiveContext();
        const WindowContext& ActiveContext() const;
        void InitializeContext(WindowContext& context, AppConfig config, std::unique_ptr<Scene> scene, bool closeStopsGame);
        void ShutdownContext(WindowContext& context);
        void RequestSceneChange(WindowContext& context, std::unique_ptr<Scene> scene, const SceneTransition& transition);
        void LoadScene(WindowContext& context, std::unique_ptr<Scene> scene);
        void UpdateContext(WindowContext& context, float deltaTime);
        void RenderContext(WindowContext& context);

        WindowContext m_MainContext;
        std::vector<WindowContext> m_AdditionalContexts;
        WindowContext* m_CurrentContext = nullptr;
    };
}
