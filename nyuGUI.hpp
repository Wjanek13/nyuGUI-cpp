#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <SDL3/SDL.h>
#include <vector>

struct HexColor {
    Uint8 r = 0;
    Uint8 g = 0;
    Uint8 b = 0;
    Uint8 a = 255;
};

inline HexColor fromHexRGB(uint32_t rgb, Uint8 alpha = 255) {
    return {
        static_cast<Uint8>((rgb >> 16) & 0xFF),
        static_cast<Uint8>((rgb >> 8) & 0xFF),
        static_cast<Uint8>(rgb & 0xFF),
        alpha
    };
}

inline HexColor fromHexRGBA(uint32_t rgba) {
    return {
        static_cast<Uint8>((rgba >> 24) & 0xFF),
        static_cast<Uint8>((rgba >> 16) & 0xFF),
        static_cast<Uint8>((rgba >> 8) & 0xFF),
        static_cast<Uint8>(rgba & 0xFF)
    };
}

inline HexColor hexToColor(uint32_t rgb, Uint8 alpha = 255) {
    return fromHexRGB(rgb, alpha);
}

struct Dim {
    float scale = 0.0f;
    int offset = 0;
};

class Widget;

struct WidgetConfig {
    Widget* parent = nullptr;
    Dim x{};
    Dim y{};
    Dim width{};
    Dim height{};
    HexColor bgColor{};
    int zIndex = 0;
    bool visible = true;
};

class Widget {
public:
    Dim x;
    Dim y;
    Dim width;
    Dim height;
    HexColor bgColor;
    int zIndex = 0;
    bool visible;

    Widget* parent = nullptr;
    std::vector<Widget*> children;

    virtual std::array<int, 4> get_real_size() const {
        std::array<int, 4> p{0, 0, 0, 0};
        if (parent) {
            p = parent->get_real_size();
        }

        int px = p[0];
        int py = p[1];
        int pw = p[2];
        int ph = p[3];

        int rx = px + static_cast<int>(pw * x.scale) + x.offset;
        int ry = py + static_cast<int>(ph * y.scale) + y.offset;
        int rw = static_cast<int>(pw * width.scale) + width.offset;
        int rh = static_cast<int>(ph * height.scale) + height.offset;

        return {rx, ry, rw, rh};
    }

    explicit Widget(WidgetConfig opts = {})
        : x(opts.x),
          y(opts.y),
          width(opts.width),
          height(opts.height),
          bgColor(opts.bgColor),
          zIndex(opts.zIndex),
          visible(opts.visible){
        if (opts.parent) {
            setParent(opts.parent);
        }
    }

    virtual ~Widget() {
        if (parent) {
            auto& siblings = parent->children;
            siblings.erase(
                std::remove(siblings.begin(), siblings.end(), this),
                siblings.end()
            );
        }

        for (auto* child : children) {
            if (child) {
                child->parent = nullptr;
            }
        }

        children.clear();
    }

    Widget* setParent(Widget* p) {
        if (parent == p || p == this) {
            return this;
        }

        if (parent) {
            auto& siblings = parent->children;
            siblings.erase(
                std::remove(siblings.begin(), siblings.end(), this),
                siblings.end()
            );
        }

        parent = p;

        if (parent) {
            parent->children.push_back(this);
        }

        return this;
    }

    Widget* setZIndex(int z) {
        zIndex = z;
        return this;
    }

    std::vector<Widget*> sortedChildren() const {
        std::vector<Widget*> ordered(children.begin(), children.end());
        std::stable_sort(
            ordered.begin(), ordered.end(),
            [](const Widget* a, const Widget* b) { return a->zIndex < b->zIndex; }
        );
        return ordered;
    }

    Widget* bringToFront() {
        if (!parent) {
            return this;
        }
        int maxZ = zIndex;
        bool tiedOrBelow = false;
        for (const auto* s : parent->children) {
            if (s == this) continue;
            maxZ = std::max(maxZ, s->zIndex);
            if (s->zIndex >= zIndex) tiedOrBelow = true;
        }
        if (tiedOrBelow) {
            zIndex = maxZ + 1;
        }
        return this;
    }

    Widget* sendToBack() {
        if (!parent) {
            return this;
        }
        int minZ = zIndex;
        bool tiedOrAbove = false;
        for (const auto* s : parent->children) {
            if (s == this) continue;
            minZ = std::min(minZ, s->zIndex);
            if (s->zIndex <= zIndex) tiedOrAbove = true;
        }
        if (tiedOrAbove) {
            zIndex = minZ - 1;
        }
        return this;
    }

    bool containsPoint(int px, int py) const {
        std::array<int, 4> r = get_real_size();
        return px >= r[0] && px < r[0] + r[2] &&
               py >= r[1] && py < r[1] + r[3];
    }

    Widget* hitTest(int px, int py) {
        auto ordered = sortedChildren();
        for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
            if (Widget* hit = (*it)->hitTest(px, py)) {
                return hit;
            }
        }
        return containsPoint(px, py) ? this : nullptr;
    }

    virtual void draw(SDL_Renderer* renderer) const {
        if (!renderer || visible == false) {
            return;
        }

        std::array<int, 4> real = get_real_size();
        SDL_FRect rect{
            static_cast<float>(real[0]),
            static_cast<float>(real[1]),
            static_cast<float>(real[2]),
            static_cast<float>(real[3])
        };

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &rect);

        for (const Widget* child : sortedChildren()) {
            if (child) {
                child->draw(renderer);
            }
        }
    }
};

struct RootConfig {
    const char* title = "My nyuGUI App";
    int width = 800;
    int height = 600;
    bool resizable = true;
    HexColor bgColor = fromHexRGB(0x000000);
};

class Root : public Widget {
private:
    bool initialized_ = false;

public:
    const char* title;
    int width;
    int height;
    bool resizable;

    std::array<int, 4> get_real_size() const override {
        int w = 0, h = 0;
        if (window) {
            SDL_GetWindowSize(window, &w, &h);
        }
        return {0, 0, w, h};
    }

    mutable SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    explicit Root(RootConfig opts = {})
        : Widget(WidgetConfig{ .bgColor = opts.bgColor }),
          title(opts.title),
          width(opts.width),
          height(opts.height),
          resizable(opts.resizable) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
            return;
        }

        window = SDL_CreateWindow(
            title,
            width,
            height,
            resizable ? SDL_WINDOW_RESIZABLE : 0
        );

        if (!window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
            SDL_Quit();
            return;
        }

        renderer = SDL_CreateRenderer(window, nullptr);

        if (!renderer) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
            SDL_DestroyWindow(window);
            window = nullptr;
            SDL_Quit();
            return;
        }

        initialized_ = true;
    }

    ~Root() override {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }

        if (window) {
            SDL_DestroyWindow(window);
        }

        if (initialized_) {
            SDL_Quit();
        }
    }

    Root(const Root&) = delete;
    Root& operator=(const Root&) = delete;

    void setBackgroundColor(uint32_t rgb, Uint8 alpha = 255) {
        bgColor = hexToColor(rgb, alpha);
    }

    void setResizable(bool enabled) {
        resizable = enabled;
        if (window) {
            SDL_SetWindowResizable(window, enabled);
        }
    }

    void run() {
        if (!window || !renderer) {
            return;
        }

        bool running = true;
        SDL_Event event;

        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }

                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
                    if (Widget* hit = hitTest(
                            static_cast<int>(event.button.x),
                            static_cast<int>(event.button.y))) {
                        if (hit != this) {
                            hit->bringToFront();
                        }
                    }
                }
            }

            SDL_SetRenderDrawColor(
                renderer,
                bgColor.r,
                bgColor.g,
                bgColor.b,
                bgColor.a
            );

            SDL_RenderClear(renderer);

            Widget::draw(renderer);

            SDL_RenderPresent(renderer);
        }
    }
};
