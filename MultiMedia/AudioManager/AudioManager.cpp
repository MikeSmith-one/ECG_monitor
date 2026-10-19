#include "AudioManager.h"

AudioManager::AudioManager(QObject* parent) 
	: QObject(parent)
{
}

AudioManager::~AudioManager()
{
}

void AudioManager::initAudio()
{
    m_soundBeep = new QSoundEffect(this);
    m_soundBeep->setSource(QUrl("qrc:/Resource/audios/beep.wav"));
    m_soundBeep->setVolume(0.5f);

    m_soundWarning = new QSoundEffect(this);
    m_soundWarning->setSource(QUrl("qrc:/Resource/audios/warning.wav"));
    m_soundWarning->setLoopCount(QSoundEffect::Infinite);
    m_soundWarning->setVolume(0.8f);

    m_soundFlatline = new QSoundEffect(this);
    m_soundFlatline->setSource(QUrl("qrc:/Resource/audios/flatline.wav"));
    m_soundFlatline->setLoopCount(QSoundEffect::Infinite);
    m_soundFlatline->setVolume(1.0f);
}

void AudioManager::playBeep() {
    if (m_soundBeep) m_soundBeep->play();
}

void AudioManager::playWarning() {
    if (m_soundFlatline) m_soundFlatline->stop();
    if (m_soundWarning && !m_soundWarning->isPlaying()) m_soundWarning->play();
}

void AudioManager::playFlatline() {
    if (m_soundWarning) m_soundWarning->stop();
    if (m_soundFlatline && !m_soundFlatline->isPlaying()) m_soundFlatline->play();
}

void AudioManager::stopAlarm() {
    if (m_soundWarning) m_soundWarning->stop();
    if (m_soundFlatline) m_soundFlatline->stop();
}