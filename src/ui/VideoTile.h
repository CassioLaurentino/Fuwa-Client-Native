#pragma once

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace fuwa::ui {

// 1 tile por participante remoto: area de video + label + botao mute/unmute
// independente. Comeca sempre mutado (requisito de produto).
class VideoTile final : public QFrame
{
    Q_OBJECT

public:
    explicit VideoTile(QString peerId, QWidget* parent = nullptr);

    const QString& peerId() const { return peerId_; }
    bool isMuted() const { return muted_; }

    // Chamado pelo MediaController quando um novo frame de video chega.
    // TODO: substituir QLabel por um widget de render real (ex. QVideoWidget
    // alimentado por um webrtc::VideoSinkInterface customizado) - o QLabel
    // aqui so existe para o layout/placeholder funcionar sem a pipeline de
    // video ligada.
    void setPlaceholderText(const QString& text);

signals:
    void muteToggled(const QString& peerId, bool muted);

private:
    void updateMuteButtonLabel();

    QString peerId_;
    bool muted_ = true; // todas as transmissoes remotas iniciam mutadas

    QLabel* videoArea_;
    QLabel* peerLabel_;
    QPushButton* muteButton_;
};

} // namespace fuwa::ui
