#include "nyuGUI.hpp"

int main() {
    Root app(RootConfig{
        .width = 960,
        .height = 640,
    });

    Widget testWidget(WidgetConfig{
        .x = Dim{0.0f, 0},
        .y = Dim{1.0f, -120},
        .width = Dim{1.0f, 0},
        .height = Dim{0.0f, 120},
        .bgColor = fromHexRGB(0x222222),
        .visible = false
    });
    testWidget.setParent(&app);
    app.run();
    return 0;
}
