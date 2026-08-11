#pragma once

#include <QDialog>

#include <vector>

class QLabel;
class QPushButton;
class QTimer;
class SDLPlayback;
class WaveformWidget;
namespace QsApi
{
    class IAudioDecoder;
}

class ManualSliceDialog final : public QDialog {
    Q_OBJECT

public:
    ManualSliceDialog(const QString &audioPath, double failedSliceStartSeconds, double failedSliceEndSeconds,
                      QWidget *parent = nullptr);
    ~ManualSliceDialog() override;

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] QString loadError() const;
    [[nodiscard]] double cutSeconds() const;
    bool writeSlices(const QString &firstPath, const QString &secondPath, QString &error) const;

private:
    void togglePlayback();
    void updatePlaybackPosition();
    void setCutSeconds(double seconds, bool seekPlayback);
    void updateLabels();

    QString m_loadError;
    std::vector<float> m_samples;
    int m_sampleRate = 44100;
    double m_durationSeconds = 0.0;
    double m_cutSeconds = 0.0;
    double m_failedStartSeconds = 0.0;
    double m_failedEndSeconds = 0.0;
    bool m_hasFailedRange = false;
    WaveformWidget *m_waveform = nullptr;
    QLabel *m_timeLabel = nullptr;
    QLabel *m_cutLabel = nullptr;
    QPushButton *m_playButton = nullptr;
    QPushButton *m_confirmButton = nullptr;
    QsApi::IAudioDecoder *m_decoder = nullptr;
    SDLPlayback *m_playback = nullptr;
    QTimer *m_timer = nullptr;
};
