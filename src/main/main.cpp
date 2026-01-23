#include "Application.h"

int main(int argc, char *argv[])
{
    ObservatoryMonitor::Application app(argc, argv);
    return app.exec();
}
