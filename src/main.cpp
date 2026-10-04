#include <QApplication>
#include <QMessageBox>

#include <mediasoupclient.hpp>

#include "AppConfig.h"
#include "ui/MainWindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    fuwa::AppConfig config;
    try
    {
        config = fuwa::AppConfig::load();
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(nullptr, "Fuwa", QString("Erro ao carregar configuracao: %1").arg(error.what()));
        return 1;
    }

    mediasoupclient::Initialize();

    fuwa::ui::MainWindow window(config);
    window.show();

    int exitCode = app.exec();

    mediasoupclient::Cleanup();
    return exitCode;
}
