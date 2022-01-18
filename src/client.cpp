#include "wofclient.h"

int main(int argc, char* argv[])
{
    WOF::ClientApp app;
    if (!app.init(argc, argv))
    {
        return 1;
    }

    app.start();

    app.deinit();

    return 0;
}