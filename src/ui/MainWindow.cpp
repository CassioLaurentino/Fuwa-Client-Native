#include "MainWindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

namespace fuwa::ui {

MainWindow::MainWindow(AppConfig config, QWidget* parent) : QMainWindow(parent), config_(std::move(config))
{
    buildUi();
}

void MainWindow::buildUi()
{
    setWindowTitle("Fuwa");
    resize(1100, 720);

    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);

    auto* topBar = new QHBoxLayout();

    connectButton_ = new QPushButton("Conectar e compartilhar tela", central);
    connect(connectButton_, &QPushButton::clicked, this, &MainWindow::connectToServer);

    qualitySelector_ = new QComboBox(central);
    qualitySelector_->addItem("720p", QVariant::fromValue(static_cast<int>(VideoQuality::P720)));
    qualitySelector_->addItem("1080p", QVariant::fromValue(static_cast<int>(VideoQuality::P1080)));
    qualitySelector_->setCurrentIndex(config_.defaultQuality == VideoQuality::P1080 ? 1 : 0);
    connect(qualitySelector_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (!mediaController_)
            return;
        auto quality = static_cast<VideoQuality>(qualitySelector_->itemData(index).toInt());
        mediaController_->setVideoQuality(quality);
    });

    statusLabel_ = new QLabel("Desconectado", central);

    topBar->addWidget(connectButton_);
    topBar->addWidget(new QLabel("Qualidade:", central));
    topBar->addWidget(qualitySelector_);
    topBar->addStretch(1);
    topBar->addWidget(statusLabel_);

    gridContainer_ = new QWidget(central);
    gridLayout_ = new QGridLayout(gridContainer_);

    auto* scrollArea = new QScrollArea(central);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(gridContainer_);

    rootLayout->addLayout(topBar);
    rootLayout->addWidget(scrollArea, 1);

    setCentralWidget(central);
}

void MainWindow::connectToServer()
{
    connectButton_->setEnabled(false);
    statusLabel_->setText("Conectando...");

    mediaController_ = std::make_unique<MediaController>(config_);

    mediaController_->onRemoteTrackAdded([this](const RemoteTrackHandle& handle) {
        QMetaObject::invokeMethod(this, [this, peerId = QString::fromStdString(handle.peerId)]() {
            addOrUpdateTile(peerId);
            statusLabel_->setText("Conectado");
        }, Qt::QueuedConnection);
    });

    mediaController_->onRemoteTrackRemoved([this](const std::string& peerId) {
        QMetaObject::invokeMethod(this, [this, id = QString::fromStdString(peerId)]() {
            removeTile(id);
        }, Qt::QueuedConnection);
    });

    mediaController_->start();
}

void MainWindow::addOrUpdateTile(const QString& peerId)
{
    if (tilesByPeerId_.count(peerId))
        return;

    auto* tile = new VideoTile(peerId, gridContainer_);
    connect(tile, &VideoTile::muteToggled, this, [this](const QString& id, bool muted) {
        if (mediaController_)
            mediaController_->setRemoteMuted(id.toStdString(), muted);
    });

    tilesByPeerId_[peerId] = tile;
    relayoutGrid();
}

void MainWindow::removeTile(const QString& peerId)
{
    auto it = tilesByPeerId_.find(peerId);
    if (it == tilesByPeerId_.end())
        return;

    it->second->deleteLater();
    tilesByPeerId_.erase(it);
    relayoutGrid();
}

void MainWindow::relayoutGrid()
{
    // Grid simples sem limite de participantes: reflui em N colunas conforme
    // a quantidade de tiles cresce (dezenas de participantes == dezenas de
    // linhas/colunas, com scroll).
    constexpr int columns = 4;

    int index = 0;
    for (auto& [peerId, tile] : tilesByPeerId_)
    {
        gridLayout_->addWidget(tile, index / columns, index % columns);
        ++index;
    }
}

} // namespace fuwa::ui
