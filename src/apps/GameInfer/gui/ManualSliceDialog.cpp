#include "ManualSliceDialog.h"

#include "Api/IAudioDecoder.h"
#include "SDLPlayback.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <audio-util/Util.h>
#include <sndfile.hh>

#include <algorithm>
#include <cmath>
#include <mutex>

namespace
{
    QString formattedTime(const double seconds) {
        const int totalMilliseconds = std::max(0, static_cast<int>(std::round(seconds * 1000.0)));
        const int minutes = totalMilliseconds / 60000;
        const int secs = totalMilliseconds / 1000 % 60;
        const int milliseconds = totalMilliseconds % 1000;
        return QStringLiteral("%1:%2.%3")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(secs, 2, 10, QLatin1Char('0'))
            .arg(milliseconds, 3, 10, QLatin1Char('0'));
    }

    bool writeWaveFile(const QString &path, const float *samples, const sf_count_t frames, const int sampleRate,
                       QString &error) {
        SndfileHandle output(path.toLocal8Bit().constData(), SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 1,
                             sampleRate);
        if (!output) {
            error = QObject::tr("Failed to create an audio slice: %1").arg(path);
            return false;
        }
        if (output.writef(samples, frames) != frames) {
            error = QObject::tr("Failed to write the complete audio slice: %1").arg(path);
            return false;
        }
        return true;
    }
}

class MemoryAudioDecoder final : public QsApi::IAudioDecoder {
public:
    MemoryAudioDecoder(const std::vector<float> &monoSamples, const int sampleRate, QObject *parent = nullptr)
        : IAudioDecoder(parent), m_monoSamples(&monoSamples),
          m_format(NAudio::WaveFormat::CreateIeeeFloatWaveFormat(sampleRate, 2)) {
        m_open = !monoSamples.empty();
    }

    bool open(const QString &, const QsMedia::WaveArguments & = {}) override { return !m_monoSamples->empty(); }
    void close() override {
        std::lock_guard lock(m_mutex);
        m_open = false;
        m_position = 0;
    }
    bool isOpen() const override { return m_open; }
    NAudio::WaveFormat inFormat() const override { return m_format; }
    NAudio::WaveFormat Format() const override { return m_format; }

    void SetPosition(const qint64 position) override {
        std::lock_guard lock(m_mutex);
        const qint64 aligned = position - position % static_cast<qint64>(sizeof(float) * 2);
        m_position = std::clamp<qint64>(aligned, 0, Length());
    }
    qint64 Position() const override {
        std::lock_guard lock(m_mutex);
        return m_position;
    }
    qint64 Length() const override {
        return static_cast<qint64>(m_monoSamples->size() * 2 * sizeof(float));
    }
    int Read(char *buffer, const int offset, const int count) override {
        std::lock_guard lock(m_mutex);
        if (!m_open || count <= 0 || offset < 0 || count % static_cast<int>(sizeof(float)) != 0 ||
            offset % static_cast<int>(sizeof(float)) != 0) {
            return 0;
        }
        const int samples = readSamples(reinterpret_cast<float *>(buffer), offset / sizeof(float),
                                        count / sizeof(float));
        return samples * sizeof(float);
    }
    int Read(float *buffer, const int offset, const int count) override {
        std::lock_guard lock(m_mutex);
        return readSamples(buffer, offset, count);
    }

private:
    int readSamples(float *buffer, const int offset, const int count) {
        if (!m_open || count <= 0 || offset < 0) {
            return 0;
        }
        const qint64 positionSamples = m_position / static_cast<qint64>(sizeof(float));
        const qint64 totalSamples = static_cast<qint64>(m_monoSamples->size() * 2);
        const qint64 availableSamples = totalSamples - positionSamples;
        const int samples = static_cast<int>(std::min<qint64>(count, std::max<qint64>(0, availableSamples)));
        if (samples <= 0) {
            return 0;
        }
        for (int index = 0; index < samples; ++index) {
            buffer[offset + index] = (*m_monoSamples)[static_cast<size_t>((positionSamples + index) / 2)];
        }
        m_position += static_cast<qint64>(samples) * sizeof(float);
        return samples;
    }

    const std::vector<float> *m_monoSamples;
    NAudio::WaveFormat m_format;
    mutable std::mutex m_mutex;
    qint64 m_position = 0;
    bool m_open = false;
};

class WaveformWidget final : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(180);
        setCursor(Qt::PointingHandCursor);
    }

    void setAudio(const std::vector<float> &samples, const double durationSeconds) {
        m_durationSeconds = durationSeconds;
        constexpr int PeakCount = 2400;
        m_peaks.assign(PeakCount, 0.0f);
        if (!samples.empty()) {
            for (int index = 0; index < PeakCount; ++index) {
                const size_t begin = static_cast<size_t>(index) * samples.size() / PeakCount;
                const size_t end = std::max(begin + 1, static_cast<size_t>(index + 1) * samples.size() / PeakCount);
                float peak = 0.0f;
                for (size_t sample = begin; sample < std::min(end, samples.size()); ++sample) {
                    peak = std::max(peak, std::abs(samples[sample]));
                }
                m_peaks[index] = std::min(1.0f, peak);
            }
        }
        update();
    }

    void setFailedRange(const double start, const double end) {
        m_failedStart = start;
        m_failedEnd = end;
        update();
    }

    void setPosition(const double seconds) {
        m_positionSeconds = std::clamp(seconds, 0.0, m_durationSeconds);
        update();
    }

signals:
    void positionSelected(double seconds);

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.fillRect(rect(), palette().window());
        const QRectF content = rect().adjusted(8, 8, -8, -8);
        painter.fillRect(content, palette().base());

        if (m_durationSeconds > 0.0 && m_failedEnd > m_failedStart) {
            const double x1 = content.left() + content.width() * m_failedStart / m_durationSeconds;
            const double x2 = content.left() + content.width() * m_failedEnd / m_durationSeconds;
            painter.fillRect(QRectF(x1, content.top(), x2 - x1, content.height()), QColor(220, 70, 70, 45));
        }

        painter.setPen(QPen(QColor(70, 150, 220), 1.0));
        const double middle = content.center().y();
        const double halfHeight = content.height() * 0.46;
        if (!m_peaks.empty()) {
            for (int x = 0; x < static_cast<int>(content.width()); ++x) {
                const size_t peakIndex = std::min(
                    m_peaks.size() - 1,
                    static_cast<size_t>(static_cast<double>(x) / std::max(1.0, content.width()) * m_peaks.size()));
                const double amplitude = m_peaks[peakIndex] * halfHeight;
                painter.drawLine(QPointF(content.left() + x, middle - amplitude),
                                 QPointF(content.left() + x, middle + amplitude));
            }
        }

        if (m_durationSeconds > 0.0) {
            const double x = content.left() + content.width() * m_positionSeconds / m_durationSeconds;
            painter.setPen(QPen(QColor(230, 160, 20), 2.0));
            painter.drawLine(QPointF(x, content.top()), QPointF(x, content.bottom()));
        }
        painter.setPen(palette().mid().color());
        painter.drawRect(content);
    }

    void mousePressEvent(QMouseEvent *event) override { selectPosition(event->position().x()); }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons().testFlag(Qt::LeftButton)) {
            selectPosition(event->position().x());
        }
    }

private:
    void selectPosition(const double x) {
        const double width = std::max(1, this->width() - 16);
        const double ratio = std::clamp((x - 8.0) / width, 0.0, 1.0);
        emit positionSelected(ratio * m_durationSeconds);
    }

    std::vector<float> m_peaks;
    double m_durationSeconds = 0.0;
    double m_positionSeconds = 0.0;
    double m_failedStart = 0.0;
    double m_failedEnd = 0.0;
};

ManualSliceDialog::ManualSliceDialog(const QString &audioPath, const double failedSliceStartSeconds,
                                     const double failedSliceEndSeconds, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("Manually split long audio"));
    resize(860, 360);

    std::string decodeError;
    auto audio = AudioUtil::resample_to_vio(std::filesystem::path(audioPath.toLocal8Bit().constData()), decodeError, 1,
                                            m_sampleRate);
    if (audio.data.byteArray.empty()) {
        m_loadError = QString::fromLocal8Bit(decodeError);
    } else {
        SndfileHandle input(audio.vio, &audio.data, SFM_READ, audio.info.format, audio.info.channels,
                            audio.info.samplerate);
        if (!input || input.frames() <= 1) {
            m_loadError = tr("Failed to decode audio for manual slicing.");
        } else {
            m_samples.resize(static_cast<size_t>(input.frames()));
            input.seek(0, SEEK_SET);
            if (input.readf(m_samples.data(), input.frames()) != input.frames()) {
                m_samples.clear();
                m_loadError = tr("Failed to read the complete audio for manual slicing.");
            }
        }
    }
    m_durationSeconds = static_cast<double>(m_samples.size()) / m_sampleRate;

    auto *layout = new QVBoxLayout(this);
    auto *description = new QLabel(
        tr("Play the audio or drag the orange playhead to a suitable cut point, then confirm. The highlighted area "
           "is the segment that exceeded the GAME limit."),
        this);
    description->setWordWrap(true);
    layout->addWidget(description);

    m_waveform = new WaveformWidget(this);
    m_waveform->setAudio(m_samples, m_durationSeconds);
    m_failedStartSeconds = std::clamp(failedSliceStartSeconds, 0.0, m_durationSeconds);
    m_failedEndSeconds = std::clamp(failedSliceEndSeconds, 0.0, m_durationSeconds);
    m_hasFailedRange = m_failedEndSeconds > m_failedStartSeconds;
    m_waveform->setFailedRange(m_failedStartSeconds, m_failedEndSeconds);
    layout->addWidget(m_waveform);

    auto *controls = new QHBoxLayout();
    m_playButton = new QPushButton(tr("Play"), this);
    m_timeLabel = new QLabel(this);
    m_cutLabel = new QLabel(this);
    controls->addWidget(m_playButton);
    controls->addWidget(m_timeLabel);
    controls->addStretch();
    controls->addWidget(m_cutLabel);
    layout->addLayout(controls);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_confirmButton = buttons->button(QDialogButtonBox::Ok);
    m_confirmButton->setText(tr("Confirm split"));
    m_confirmButton->setEnabled(isValid());
    layout->addWidget(buttons);

    setCutSeconds(m_hasFailedRange ? (m_failedStartSeconds + m_failedEndSeconds) / 2.0
                                   : m_durationSeconds / 2.0,
                  false);

    m_decoder = new MemoryAudioDecoder(m_samples, m_sampleRate, this);
    m_playback = new SDLPlayback(this);
    const bool playbackSetup = m_playback->setup(QsMedia::PlaybackArguments{44100, 2, 1024});
    const bool playbackReady = playbackSetup && m_decoder->isOpen();
    if (playbackReady) {
        m_playback->setDecoder(m_decoder);
        m_decoder->SetCurrentTime(static_cast<qint64>(std::round(m_cutSeconds * 1000.0)));
    } else {
        m_playButton->setEnabled(false);
        m_playButton->setToolTip(tr("Audio playback is unavailable; the playhead can still be dragged."));
    }

    m_timer = new QTimer(this);
    m_timer->setInterval(30);
    connect(m_timer, &QTimer::timeout, this, &ManualSliceDialog::updatePlaybackPosition);
    connect(m_playButton, &QPushButton::clicked, this, &ManualSliceDialog::togglePlayback);
    connect(m_waveform, &WaveformWidget::positionSelected, this,
            [this](const double seconds) { setCutSeconds(seconds, true); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ManualSliceDialog::~ManualSliceDialog() {
    if (m_playback != nullptr && m_playback->isAvailable()) {
        m_playback->stop();
        m_playback->dispose();
    }
    if (m_decoder != nullptr) {
        m_decoder->close();
    }
}

bool ManualSliceDialog::isValid() const { return m_loadError.isEmpty() && m_samples.size() > 2; }

QString ManualSliceDialog::loadError() const { return m_loadError; }

double ManualSliceDialog::cutSeconds() const { return m_cutSeconds; }

void ManualSliceDialog::togglePlayback() {
    if (m_playback == nullptr || m_decoder == nullptr || !m_decoder->isOpen()) {
        return;
    }
    if (m_playback->isPlaying()) {
        m_playback->stop();
        m_timer->stop();
    } else {
        if (m_decoder->CurrentTime() >= m_decoder->TotalTime()) {
            m_decoder->SetCurrentTime(0);
        }
        m_playback->play();
        m_timer->start();
    }
    m_playButton->setText(m_playback->isPlaying() ? tr("Pause") : tr("Play"));
}

void ManualSliceDialog::updatePlaybackPosition() {
    if (m_decoder == nullptr || !m_decoder->isOpen()) {
        return;
    }
    setCutSeconds(static_cast<double>(m_decoder->CurrentTime()) / 1000.0, false);
    if (m_playback != nullptr && !m_playback->isPlaying()) {
        m_timer->stop();
        m_playButton->setText(tr("Play"));
    }
}

void ManualSliceDialog::setCutSeconds(const double seconds, const bool seekPlayback) {
    const double margin = std::min(0.1, m_durationSeconds / 4.0);
    m_cutSeconds = std::clamp(seconds, margin, std::max(margin, m_durationSeconds - margin));
    m_waveform->setPosition(m_cutSeconds);
    if (seekPlayback && m_decoder != nullptr && m_decoder->isOpen()) {
        m_decoder->SetCurrentTime(static_cast<qint64>(std::round(m_cutSeconds * 1000.0)));
    }
    if (m_confirmButton != nullptr) {
        const bool insideFailedRange = !m_hasFailedRange ||
                                       (m_cutSeconds > m_failedStartSeconds && m_cutSeconds < m_failedEndSeconds);
        m_confirmButton->setEnabled(isValid() && insideFailedRange);
        m_confirmButton->setToolTip(insideFailedRange
                                        ? QString()
                                        : tr("Choose a cut point inside the highlighted overlong segment."));
    }
    updateLabels();
}

void ManualSliceDialog::updateLabels() {
    m_timeLabel->setText(tr("%1 / %2").arg(formattedTime(m_cutSeconds), formattedTime(m_durationSeconds)));
    m_cutLabel->setText(tr("Cut at %1 · Parts: %2 / %3")
                            .arg(formattedTime(m_cutSeconds), formattedTime(m_cutSeconds),
                                 formattedTime(m_durationSeconds - m_cutSeconds)));
}

bool ManualSliceDialog::writeSlices(const QString &firstPath, const QString &secondPath, QString &error) const {
    if (!isValid()) {
        error = m_loadError;
        return false;
    }
    const sf_count_t splitFrame = std::clamp<sf_count_t>(
        static_cast<sf_count_t>(std::round(m_cutSeconds * m_sampleRate)), 1,
        static_cast<sf_count_t>(m_samples.size() - 1));
    const sf_count_t totalFrames = static_cast<sf_count_t>(m_samples.size());
    const QString firstTemporary = firstPath + QStringLiteral(".partial.wav");
    const QString secondTemporary = secondPath + QStringLiteral(".partial.wav");
    QFile::remove(firstTemporary);
    QFile::remove(secondTemporary);

    if (!writeWaveFile(firstTemporary, m_samples.data(), splitFrame, m_sampleRate, error) ||
        !writeWaveFile(secondTemporary, m_samples.data() + splitFrame, totalFrames - splitFrame, m_sampleRate, error)) {
        QFile::remove(firstTemporary);
        QFile::remove(secondTemporary);
        return false;
    }
    if (!QFile::rename(firstTemporary, firstPath)) {
        QFile::remove(firstTemporary);
        QFile::remove(secondTemporary);
        error = tr("Failed to finalize the first audio slice: %1").arg(firstPath);
        return false;
    }
    if (!QFile::rename(secondTemporary, secondPath)) {
        QFile::remove(firstPath);
        QFile::remove(secondTemporary);
        error = tr("Failed to finalize the second audio slice: %1").arg(secondPath);
        return false;
    }
    return true;
}

#include "ManualSliceDialog.moc"
