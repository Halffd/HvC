#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "WindowManager.hpp"
#include "Rect.hpp"

namespace H {
    class Window : public WindowManager {
    public:
        wID id;

        #ifdef __linux__
        static DisplayServer displayServer;
        #endif

        Window(cstr identifier, const int method = 1);

        wID Find2(cstr identifier, cstr type = "title");
        template<typename T> wID FindT(const T& identifier);

        str Title(wID win = 0);
        bool Active(wID win = 0);
        bool Exists(wID win);

        void Activate(wID win = 0);
        void Close(wID win = 0);
        void Min(wID win = 0);
        void Max(wID win = 0);
        Rect Pos(wID win = 0);
        void AlwaysOnTop(wID win = 0, bool top = true);
        void Transparency(wID win = 0, int alpha = 255);

    private:
        Rect GetPositionX11(wID win);
        Rect GetPositionWayland(wID win);

        #ifdef __linux__
        DisplayServer DetectDisplayServer();
        #else
        Rect GetPositionWindows(wID win);
        #endif
    };
}

#endif // WINDOW_HPP
