#include "TurnTable.h"
#include <math.h>

using namespace GE;


CScratchDisc::CScratchDisc(GE::CAudioBuffer *discSource)
{
    m_source = discSource;
    m_pos = 0;
    m_speed = 0.0f;
    m_targetSpeed = 0.0f;
    m_cc = 0;
    m_headOn = false;

    memset(m_lp, 0, sizeof(int) * 2);
    memset(m_bp, 0, sizeof(int) * 2);
    memset(m_hp, 0, sizeof(int) * 2);


    setResonance(1.0f);
    setCutOff(1.0f);

    m_cutOffValue = m_cutOffTarget;
    m_resonanceValue = m_resonanceTarget;
}


CScratchDisc::~CScratchDisc()
{
    if(m_source != NULL) {
        delete m_source;
        m_source = NULL;
    }
}


void CScratchDisc::setCutOff( float cutoff ) {
    m_cutOffTarget = cutoff; //powf(cutoff, 2.0f );
}


void CScratchDisc::setResonance(float resonance) {

    m_resonanceTarget = powf(resonance, 2.0f);
}



void CScratchDisc::setSpeed(float speed)
{
    if(speed > -100.0f && speed < 100.0f) {
        m_targetSpeed = speed;
    }
}


void CScratchDisc::aimSpeed(float speed, float power)
{
    if(speed > -100.0f && speed < 100.0f) {
        m_targetSpeed = m_targetSpeed * (1.0f-power) + speed*power;
    }
}


int CScratchDisc::pullAudio(AUDIO_SAMPLE_TYPE *target, int bufferLength)
{
    if(m_source == NULL) {
        return 0;
    }

    AUDIO_SAMPLE_TYPE *t_target = target + bufferLength;
    SAMPLE_FUNCTION_TYPE sfunc = m_source->getSampleFunction();

    int channelLength = ((m_source->getDataLength()) / (m_source->getNofChannels() * m_source->getBytesPerSample())) - 2;
    channelLength<<=11;

    int p;
    int fixedReso = (m_resonanceValue * 4096.0f);
    int fixedCutoff = (m_cutOffValue * 4096.0f);

    float speedmul = (float)m_source->getSamplesPerSec() / (float)AUDIO_FREQUENCY * 2048.0f;
    int inc = (int)(m_speed * speedmul);

    if (m_headOn == false) {
        m_pos = bufferLength / 2 * inc;
        return 0;
    }

    int input;
    while(target != t_target) {
        if(m_cc > 64) {
            m_speed += (m_targetSpeed - m_speed) * 0.1f;
            m_cutOffValue += (m_cutOffTarget - m_cutOffValue) * 0.1f;
            m_resonanceValue += (m_resonanceTarget - m_resonanceValue) * 0.1f;
            inc = (int)(m_speed * speedmul);
            fixedReso = (m_resonanceValue * 4096.0f);
            fixedCutoff = (m_cutOffValue * 4096.0f);
            m_cc = 0;
        }
        else {
            m_cc++;
        }

        if(m_pos >= channelLength) {
            m_pos %= channelLength;
        }

        if(m_pos < 0) {
            m_pos = channelLength - 1 - ((-m_pos) % channelLength);
        }

        p = (m_pos >> 11);



        input = (((sfunc)(m_source, p, 0) * (2047^(m_pos & 2047)) + (sfunc)(m_source, p+1, 0) * (m_pos & 2047)) >> 11);
        m_lp[0] += ((m_bp[0] * fixedCutoff) >> 12);
        m_hp[0] = input - m_lp[0] - ((m_bp[0] * fixedReso) >> 12);
        m_bp[0] += ((m_hp[0] * fixedCutoff) >> 12);

        input = m_lp[0];
        if(input < -32767) {
            input = -32767;
        }
        if(input > 32767) {
            input = 32767;
        }

        target[0] = input;


        input = (((sfunc)(m_source, p, 1) * (2047 ^ (m_pos & 2047)) + (sfunc)(m_source, p+1, 1) * (m_pos & 2047)) >> 11);
        m_lp[1] += ((m_bp[1] * fixedCutoff) >> 12);
        m_hp[1] = input - m_lp[1] - ((m_bp[1] * fixedReso) >> 12);
        m_bp[1] += ((m_hp[1] * fixedCutoff) >> 12);

        input = m_lp[1];
        if(input < -32767) {
            input = -32767;
        }
        if(input > 32767) {
            input = 32767;
        }

        target[1] = input;
        target += 2;
        m_pos += inc;
    }

    return bufferLength;
}


TurnTable::TurnTable()
{
    m_discSample = GE::CAudioBuffer::loadWav(QString(":/sounds/melody.wav"));

    m_sdisc = new CScratchDisc(m_discSample);   // ownership is taken
    m_audioMixer.addAudioSource(m_sdisc);
    m_audioOut = new GE::AudioOut(this, &m_audioMixer);

    m_audioMixer.setGeneralVolume(0.4999f);
}


void TurnTable::addAudioSource(GE::IAudioSource *source)
{
    m_audioMixer.addAudioSource(source);
}


void TurnTable::setDiscAimSpeed(QVariant speed)
{
    m_sdisc->aimSpeed(speed.toFloat());
}


void TurnTable::setDiscSpeed(QVariant speed)
{
    m_sdisc->setSpeed(speed.toFloat());
}


TurnTable::~TurnTable()
{
    if(m_audioOut) {
        delete m_audioOut;
        m_audioOut = NULL;
    }

    if(m_sdisc != NULL) {
        delete m_sdisc;
        m_sdisc = NULL;
    }

    m_discSample = NULL;
}
