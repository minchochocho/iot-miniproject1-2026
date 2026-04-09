#include "CodeShelf.h"
#include <QtWidgets/QApplication>
#include<QSQlDatabase>

int main(int argc, char *argv[])
    {
    QApplication app(argc, argv);
    CodeShelf window;
    window.show();
    return app.exec();

}
