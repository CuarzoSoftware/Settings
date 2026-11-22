#include <CZ/Marco/MApp.h>
#include <Window/SettingsWindow.h>

using namespace CZ;

int main(int argc, char *argv[])
{
    // Displays: wlr-output-management
    MOptions options;
    options.extensions.add(MOutputManagerExt);
    auto app { MApp::Make(options) };

    if (!app)
        return 1;

    // Matches the desktop entry (dock icon, window grouping).
    app->setAppId("org.cuarzo.Settings");

    SettingsWindow window;

    // E.g. `cz-settings displays`: opens that section (unknown ones are ignored)
    if (argc > 1)
        window.show(argv[1]);

    // Like macOS's System Settings: closing the window quits the app.
    window.onMappedChanged.subscribe(&window, [&window, &app]{
        if (!window.mapped())
            app->exit();
    });

    window.setMapped(true);
    return app->run();
}
