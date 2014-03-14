#include "mainwindow.h"
#include <ctime>

int main(int argc, char * argv[]) {
    srand(time(NULL));
    MainWindow *window = new MainWindow();
    return window->start();
}
