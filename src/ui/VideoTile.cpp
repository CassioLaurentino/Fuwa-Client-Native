#include "VideoTile.h"

namespace fuwa::ui {

VideoTile::VideoTile(QString peerId, QWidget* parent) : QFrame(parent), peerId_(std::move(peerId))
{
    setFrameShape(QFrame::StyledPanel);
    setMinimumSize(240, 160);

    videoArea_ = new QLabel(this);
    videoArea_->setAlignment(Qt::AlignCenter);
    videoArea_->setStyleSheet("background-color: #111; color: #888;");
    videoArea_->setText("Conectando...");
    videoArea_->setMinimumHeight(135);

    peerLabel_ = new QLabel(peerId_, this);
    peerLabel_->setAlignment(Qt::AlignLeft);

    muteButton_ = new QPushButton(this);
    connect(muteButton_, &QPushButton::clicked, this, [this]() {
        muted_ = !muted_;
        updateMuteButtonLabel();
        emit muteToggled(peerId_, muted_);
    });
    updateMuteButtonLabel();

    auto* footer = new QHBoxLayout();
    footer->addWidget(peerLabel_, 1);
    footer->addWidget(muteButton_);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(videoArea_, 1);
    layout->addLayout(footer);
}

void VideoTile::setPlaceholderText(const QString& text)
{
    videoArea_->setText(text);
}

void VideoTile::updateMuteButtonLabel()
{
    muteButton_->setText(muted_ ? QStringLiteral("\xF0\x9F\x94\x87 Mutado") : QStringLiteral("\xF0\x9F\x94\x8A Ativo"));
}

} // namespace fuwa::ui
