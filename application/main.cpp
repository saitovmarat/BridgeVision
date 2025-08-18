#include <QApplication>
#include "mainWindow.h"

#include "iLink.h"

#ifdef UDP
#include "udpLink.h"
using Link = UdpLink;
#else
#include "udpLink.h"
using Link = UdpLink;
#endif

int main(int argc, char* argv[])
{
    const QApplication a(argc, argv);

    ILink* link = new Link();
    MainWindow w(link);
    w.show();
    return a.exec();
}
