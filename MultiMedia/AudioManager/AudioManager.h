#pragma once

#include <QObject>
#include <QSoundEffect>
#include <QUrl>

class AudioManager : public QObject
{
    Q_OBJECT
public:
    explicit AudioManager(QObject* parent = nullptr);
    ~AudioManager();

public slots:
    // 在子线程初始化音效
    void initAudio();

    void playBeep();

    void playWarning();

    void playFlatline();

    void stopAlarm();

private:
    QSoundEffect* m_soundBeep = nullptr;
    QSoundEffect* m_soundWarning = nullptr;
    QSoundEffect* m_soundFlatline = nullptr;
};