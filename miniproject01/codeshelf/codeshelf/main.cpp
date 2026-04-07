#include "codeshelf.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    CodeShelf window;
    window.show();
    return app.exec();

}
