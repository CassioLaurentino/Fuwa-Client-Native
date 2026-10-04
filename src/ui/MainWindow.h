#pragma once

#include <map>
#include <memory>

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>

#include "../AppConfig.h"
#include "../MediaController.h"
#include "VideoTile.h"

namespace fuwa::ui {

// Janela principal: tela de conexao + grid dinamico (sem limite de tiles) +
// seletor de qualidade de video. E a unica classe que conhece tanto o Qt
// quanto o MediaController - mantem a UI "burra" (so reage a eventos).
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AppConfig config, QWidget* parent = nullptr);

private:
    void buildUi();
    void connectToServer();
    void addOrUpdateTile(const QString& peerId);
    void removeTile(const QString& peerId);
    void relayoutGrid();

    AppConfig config_;
    std::unique_ptr<MediaController> mediaController_;

    QLabel* statusLabel_;
    QPushButton* connectButton_;
    QComboBox* qualitySelector_;
    QWidget* gridContainer_;
    QGridLayout* gridLayout_;

    std::map<QString, VideoTile*> tilesByPeerId_;
};

} // namespace fuwa::ui
