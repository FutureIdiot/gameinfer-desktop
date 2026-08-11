#include "ManualSliceDialog.h"

#include <QApplication>
#include <QDir>
#include <QTemporaryDir>

#include <sndfile.hh>

#include <cmath>
#include <iostream>
#include <vector>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        return 1;
    }

    constexpr int SampleRate = 44100;
    std::vector<float> samples(SampleRate);
    for (size_t index = 0; index < samples.size(); ++index) {
        samples[index] = static_cast<float>(0.25 * std::sin(2.0 * 3.141592653589793 * 440.0 * index / SampleRate));
    }
    const QString source = QDir(temporary.path()).filePath(QStringLiteral("source.wav"));
    {
        SndfileHandle output(source.toLocal8Bit().constData(), SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 1,
                             SampleRate);
        if (!output || output.writef(samples.data(), samples.size()) != static_cast<sf_count_t>(samples.size())) {
            std::cerr << "failed to create source audio" << std::endl;
            return 1;
        }
    }

    ManualSliceDialog dialog(source, 0.2, 0.8);
    if (!dialog.isValid() || std::abs(dialog.cutSeconds() - 0.5) > 0.01) {
        std::cerr << "manual slice dialog failed to load audio: " << dialog.loadError().toStdString() << std::endl;
        return 1;
    }
    const QString first = QDir(temporary.path()).filePath(QStringLiteral("first.wav"));
    const QString second = QDir(temporary.path()).filePath(QStringLiteral("second.wav"));
    QString error;
    if (!dialog.writeSlices(first, second, error)) {
        std::cerr << "manual split failed: " << error.toStdString() << std::endl;
        return 1;
    }
    const SndfileHandle firstInput(first.toLocal8Bit().constData());
    const SndfileHandle secondInput(second.toLocal8Bit().constData());
    if (!firstInput || !secondInput || std::abs(firstInput.frames() - SampleRate / 2) > 2 ||
        std::abs(secondInput.frames() - SampleRate / 2) > 2) {
        std::cerr << "manual split durations are incorrect" << std::endl;
        return 1;
    }

    std::cout << "GameInfer manual slice tests passed" << std::endl;
    return 0;
}
