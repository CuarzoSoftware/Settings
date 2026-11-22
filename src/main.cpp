#include <CZ/Marco/MApp.h>
#include <Window/SettingsWindow.h>

using namespace CZ;

int main()
{
    auto app { MApp::Make() };

    if (!app)
        return 1;

    // Matches the desktop entry (dock icon, window grouping).
    app->setAppId("org.cuarzo.Settings");

    SettingsWindow window;

    // Like macOS's System Settings: closing the window quits the app.
    window.onMappedChanged.subscribe(&window, [&window, &app]{
        if (!window.mapped())
            app->exit();
    });

    window.setMapped(true);
    return app->run();
}
