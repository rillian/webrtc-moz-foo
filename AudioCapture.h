/*
   Uses the webrtc capture routines to record audio to a file.
 */

#ifndef AudioCaptureGIPS_H
#define AudioCaptureGIPS_H

#include <stdio.h>

#include <voe_base.h>
#include <voe_hardware.h>
#include <voe_codec.h>
#include <voe_external_media.h>

#include <common_types.h>


/*** Audio capture controller ***/

class AudioSourceGIPS : public webrtc::VoEMediaProcess {
public:
    AudioSourceGIPS();
    ~AudioSourceGIPS();
    void Start();
    void Stop();

    void Process(const int channel,
                 const webrtc::ProcessingTypes type,
                 WebRtc_Word16 audio10ms[], const int length,
                 const int samplingFreq, const bool isStereo);

protected:
    const char *filename;
    int frames;
    webrtc::VoiceEngine* ptrVoE;
    webrtc::VoEBase* ptrVoEBase;
    webrtc::VoEHardware* ptrVoEHardware;
    webrtc::VoEExternalMedia* ptrVoERender;
    webrtc::VoECodec* ptrVoECodec;
    webrtc::CodecInst codec;
    int channel;

    int WriteWAVHeader(int samplingFreq, int channels);
    int FinishWAVHeader();
    FILE *wav;
};

#endif // AudioCaptureGIPS_H
