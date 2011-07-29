/*
   Uses the webrtc capture routines to record audio to a file.
 */

#include <string.h>

#include "AudioCapture.h"

/** implementation **/

/* dummy transport to act as a sink for the voice engine pipeline **/
class DummyTransport : public webrtc::Transport {
public:
    int SendPacket(int channel, const void *data, int len);
    int SendRTCPPacket(int channel, const void *data, int len);
};

int
DummyTransport::SendPacket(int channel, const void *data, int len)
{
    fprintf(stderr,
        "RTP packet channel %d 0x%016llx %d bytes\n", channel,
        (unsigned long long)data, len);

    return 0;
}

int
DummyTransport::SendRTCPPacket(int channel, const void *data, int len)
{
    fprintf(stderr,
        "RTCP packet channel %d 0x%016llx %d bytes\n", channel,
        (unsigned long long)data, len);

    return 0;
}

/* our dummy transport has no state, so it's safe to use a global
   for the version we pass to VoENetwork. This allows us to keep
   the class definition private to our implementation, since its
   lifecycle isn't tied to any AudioSourceGIPS object */
static DummyTransport dummyTransport;


AudioSourceGIPS::AudioSourceGIPS()
{
    int error = 0;

    filename = "test.wav";

    ptrVoE = webrtc::VoiceEngine::Create();
    if (ptrVoE == NULL) {
        fprintf(stderr, "ERROR in GIPSVoiceEngine::Create\n");
        return;
    }

    error = ptrVoE->SetTraceFile("GIPSVoETrace.txt");
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSVoiceEngine::SetTraceFile\n");
        return;
    }

    ptrVoEBase = webrtc::VoEBase::GetInterface(ptrVoE);
    if (ptrVoEBase == NULL) {
        fprintf(stderr, "ERROR in GIPSVoEBase::GetInterface\n");
        return;
    }
    error = ptrVoEBase->Init();
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSVoEBase::Init\n");
        return;
    }

    channel = ptrVoEBase->CreateChannel();
    if (channel < 0) {
        fprintf(stderr, "ERROR in GIPSVoEBase::CreateChannel\n");
    }

    ptrVoEHardware = webrtc::VoEHardware::GetInterface(ptrVoE);
    if (ptrVoEHardware == NULL) {
        fprintf(stderr, "ERROR in VoEHardware::GetInterface\n");
    }

    int nDevices;
    error = ptrVoEHardware->GetNumOfRecordingDevices(nDevices);
    fprintf(stderr, "Got %d recording devices devices!\n", nDevices);
    for (int i = 0; i < nDevices; i++) {
        char name[128], guid[128];
        memset(name, 0, 128);
        memset(guid, 0, 128);
        error = ptrVoEHardware->GetRecordingDeviceName(i, name, guid);
        if (error) {
            fprintf(stderr, "ERROR in VoEHardware::GetRecordingDeviceName\n");
        } else {
            fprintf(stderr, "%d\t%s (%s)\n", i, name, guid);
        }
    }

    fprintf(stderr, "setting default recording device\n");
    error = ptrVoEHardware->SetRecordingDevice(0);
    if (error) {
        fprintf(stderr, "ERROR in VoEHardware::SetRecordingDevice\n");
    }

    bool recAvail = false;
    ptrVoEHardware->GetRecordingDeviceStatus(recAvail);
    fprintf(stderr, "Recording device is %savailable\n",
        recAvail ? "" : "NOT ");

    ptrVoERender = webrtc::VoEExternalMedia::GetInterface(ptrVoE);
    if (ptrVoERender == NULL) {
        fprintf(stderr, "ERROR in GIPSVoEExernalMedia::GetInterface\n");
        return;
    }

    /* NB: To capture audio we must have a network sink
       for the voice engine packets; it won't just run
       the capture part of the pipeline like the video engine
       will. We install a dummy network transport object
       to handle this. The alternative it to set a destination
       socket with ptrVoEBase->SetSendDestination, after
       which StartSend() to push real network packets. */
    //ptrVoEBase->SetSendDestination(channel, 8000, "127.0.0.1");
    //ptrVoEBase->SetLocalReceiver(channel, 8000);

    ptrVoENetwork = webrtc::VoENetwork::GetInterface(ptrVoE);
    if (ptrVoENetwork == NULL) {
        fprintf(stderr, "ERROR in GIPSVoENetwork::GetInterface\n");
    }

    error = ptrVoENetwork->RegisterExternalTransport(channel, dummyTransport);
    if (error) {
        fprintf(stderr,
            "ERROR in GIPSVoENetwork::RegisterExternalTransport\n");
    }

    /* set up the codec we want to use */
    ptrVoECodec = webrtc::VoECodec::GetInterface(ptrVoE);
    if (ptrVoECodec == NULL) {
        fprintf(stderr, "ERROR in GIPSVoECodec::GetInterface\n");
    } else {
        strcpy(codec.plname, "PCMU");
        codec.channels = 1;
        codec.pacsize = 160;
        codec.plfreq = 8000;
        codec.pltype = 0;
        codec.rate = 64000;
        error = ptrVoECodec->SetSendCodec(channel, codec);
        if (error) {
            fprintf(stderr, "ERROR in GIPSVoECodec::SetSendCodec\n");
        }
    }
}

AudioSourceGIPS::~AudioSourceGIPS()
{
    int error;

    error = ptrVoENetwork->DeRegisterExternalTransport(channel);
    if (error) {
        fprintf(stderr,
            "ERROR in GIPSVoENetwork::DeRegisterExternalTransport\n");
    }

    error = ptrVoEBase->DeleteChannel(channel);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSVoEBase::DeleteChannel\n");
    }

    ptrVoEBase->Terminate();
    ptrVoEBase->Release();
    ptrVoEHardware->Release();
    ptrVoECodec->Release();
    ptrVoERender->Release();
    ptrVoENetwork->Release();

    if (webrtc::VoiceEngine::Delete(ptrVoE) == false) {
        fprintf(stderr, "ERROR in GIPSVoiceEngine::Delete\n");
    }
}

void
AudioSourceGIPS::Start()
{
    int error;

    wav = fopen(filename, "wb");
    if (wav == NULL) {
        fprintf(stderr, "ERROR opening audio spool file\n");
    }
    this->WriteWAVHeader(8000,1);

#if 0
    error = ptrVoEBase->StartReceive(channel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEBase::StartReceive\n");
    }
    error = ptrVoEBase->StartPlayout(channel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEBase::StartPlayout\n");
    }
#endif
    error = ptrVoEBase->StartSend(channel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEBase::StartSend\n");
    }

    error = ptrVoERender->RegisterExternalMediaProcessing(channel,
        webrtc::kRecordingPerChannel, *this);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEExternalMedia::RegisterExternalMediaProcessing\n");
    }
}

void
AudioSourceGIPS::Stop()
{
    int error;

    error = ptrVoERender->DeRegisterExternalMediaProcessing(channel,
        webrtc::kRecordingPerChannel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEExternalMedia::DeRegisterExternalMediaProcessing\n");
    }

    error = ptrVoEBase->StopSend(channel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEBase::StopSend\n");
    }
    error = ptrVoEBase->StopReceive(channel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEBase::StopReceive\n");
    }
    error = ptrVoEBase->StopPlayout(channel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEBase::StopPlayout\n");
    }

    this->FinishWAVHeader();
    fclose(wav);

}

void AudioSourceGIPS::Process(const int channel,
             const webrtc::ProcessingTypes type,
             WebRtc_Word16 audio10ms[], const int length,
             const int samplingFreq, const bool isStereo)
{
    fprintf(stderr, "Audio frame %08d buffer 0x%016llx\t %d\t %d Hz %s\n",
            frames,
            (unsigned long long)audio10ms, length, samplingFreq,
            isStereo ? "stereo" : "mono");
    frames++;

    fwrite(audio10ms, length, 1, wav);
    return;
}

/* helper: write a little-endian 32 bit integer */
static int writeu32(unsigned int value, FILE *out)
{
    unsigned char buf[4];

    buf[0] = value & 0xff;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 24) & 0xff;

    return fwrite(buf, 4, 1, out);
}

/* helper: write a little-endian 16 bit integer */
static int writeu16(unsigned short value, FILE *out)
{
    unsigned char buf[2];

    buf[0] = value & 0xff;
    buf[1] = (value >> 8) & 0xff;

    return fwrite(buf, 2, 1, out);
}

/* write out a WAV file header for spooling audio */
int AudioSourceGIPS::WriteWAVHeader(int samplingFreq, int channels)
{
    int bytesPerChannel = 2;

    // RIFF header
    fwrite("RIFF", 4, 1, wav);
    writeu32(36, wav); // chunk size; fixup later with the total length
    fwrite("WAVE", 4, 1, wav);
    // fmt chunk
    fwrite("fmt ", 4, 1, wav);
    writeu32(16, wav); // chunk length for PCM format
    writeu16(1, wav);  // PCM format
    writeu16(channels, wav);
    writeu32(samplingFreq, wav);
    writeu32(samplingFreq*channels*bytesPerChannel, wav); // bytes per second
    writeu16(channels*bytesPerChannel, wav);              // frame size
    writeu16(bytesPerChannel*8, wav);                     // bits per channel
    // data chunk
    fwrite("data", 4, 1, wav);
    writeu32(0, wav);  // data size; fixup later

    return 0;
}

/* Fix up WAV file header with final length */
int AudioSourceGIPS::FinishWAVHeader()
{
    long length;

    // find out how much data we've written
    length = ftell(wav);
    // update the size fields appropriately
    fseek(wav, 4, SEEK_SET);
    writeu32(length - 8, wav);
    fseek(wav, 40, SEEK_SET);
    writeu32(length - 44, wav);

    return 0;
}
