/* Test client
   Uses the webrtc capture routines to record video to a file.
 */

#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>

#include <vie_base.h>
#include <vie_codec.h>
#include <vie_render.h>
#include <vie_capture.h>

#include <voe_base.h>
#include <voe_hardware.h>
#include <voe_external_media.h>

#include <common_types.h>


/*** Video capture controller ***/

class VideoSourceGIPS : public webrtc::ExternalRenderer {
public:
    VideoSourceGIPS();
    ~VideoSourceGIPS();
    void Start();
    void Stop();

protected:
    FILE *tmp;
    int videoChannel;
    int gipsCaptureId;
    webrtc::VideoEngine* ptrViE;
    webrtc::ViEBase* ptrViEBase;
    webrtc::ViECapture* ptrViECapture;
    webrtc::ViERender* ptrViERender;
    webrtc::CaptureCapability cap;
    int frames;

    // GIPSViEExternalRenderer
    int FrameSizeChange(
        unsigned int width, unsigned int height, unsigned int numberOfStreams
    );
    int DeliverFrame(unsigned char* buffer, int bufferSize);
};

VideoSourceGIPS::VideoSourceGIPS()
{
    int error = 0;
    videoChannel = -1;

    // Open file for recording
    tmp = fopen("test.y4m", "w+");
    ptrViE = webrtc::VideoEngine::Create();
    if (ptrViE == NULL) {
        fprintf(stderr, "ERROR in GIPSVideoEngine::Create\n");
        return;
    }

    error = ptrViE->SetTraceFile("GIPSViETrace.txt");
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSVideoEngine::SetTraceFile\n");
        return;
    }

    ptrViEBase = webrtc::ViEBase::GetInterface(ptrViE);
    if (ptrViEBase == NULL) {
        fprintf(stderr, "ERROR in GIPSViEBase::GIPSViE_GetInterface\n");
        return;
    }
    error = ptrViEBase->Init();
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViEBase::GIPSViE_Init\n");
        return;
    }

    error = ptrViEBase->CreateChannel(videoChannel);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViEBase::GIPSViE_CreateChannel\n");
        return;
    }

    /* List available capture devices, allocate and connect. */
    ptrViECapture =
        webrtc::ViECapture::GetInterface(ptrViE);
    if (ptrViEBase == NULL) {
        fprintf(stderr, "ERROR in GIPSViECapture::GIPSViE_GetInterface\n");
        return;
    }

    if (ptrViECapture->NumberOfCaptureDevices() <= 0) {
        fprintf(stderr, "ERROR no video devices found\n");
        return;
    }

    const unsigned int KMaxDeviceNameLength = 128;
    const unsigned int KMaxUniqueIdLength = 256;
    char deviceName[KMaxDeviceNameLength];
    memset(deviceName, 0, KMaxDeviceNameLength);
    char uniqueId[KMaxUniqueIdLength];
    memset(uniqueId, 0, KMaxUniqueIdLength);

    error = ptrViECapture->GetCaptureDevice(0, deviceName,
        KMaxDeviceNameLength, uniqueId, KMaxUniqueIdLength);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViECapture::GIPSViE_GetCaptureDevice\n");
        return;
    }
    fprintf(stderr, "Got device name='%s' id='%s'\n", deviceName, uniqueId);

    int numCaps = ptrViECapture->NumberOfCapabilities(
        uniqueId, KMaxUniqueIdLength
    );
    fprintf(stderr, "Got %d capabilities!\n", numCaps);

    for (int i = 0; i < numCaps; i++) {
        error = ptrViECapture->GetCaptureCapability(
            uniqueId, KMaxUniqueIdLength, i, cap
        );
        fprintf(stderr,
            "%d. w=%d, h=%d, fps=%d, d=%d, i=%d, RGB24=%d, i420=%d, ARGB=%d\n",
            i, cap.width, cap.height,
            cap.maxFPS, cap.expectedCaptureDelay, cap.interlaced,
            cap.rawType == webrtc::kVideoRGB24,
            cap.rawType == webrtc::kVideoI420,
            cap.rawType == webrtc::kVideoARGB);
    }

    // Set to first capability
    error = ptrViECapture->GetCaptureCapability(
        uniqueId, KMaxUniqueIdLength, 0, cap
    );

    // Write out video header
    fprintf(tmp, "YUV4MPEG2 W%d H%d F%d:1\n",
        cap.width, cap.height, cap.maxFPS);

    gipsCaptureId = 0;
    error = ptrViECapture->AllocateCaptureDevice(uniqueId,
        KMaxUniqueIdLength, gipsCaptureId);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViECapture::GIPSViE_AllocateCaptureDevice\n");
        return;
    }
}

VideoSourceGIPS::~VideoSourceGIPS()
{
    fclose(tmp);
    int error = ptrViECapture->ReleaseCaptureDevice(gipsCaptureId);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViECapture::GIPSViE_ReleaseCaptureDevice\n");
    }

    error = ptrViEBase->DeleteChannel(videoChannel);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViEBase::GIPSViE_DeleteChannel\n");
    }

    ptrViECapture->Release();
    ptrViEBase->Release();

    if (webrtc::VideoEngine::Delete(ptrViE) == false) {
        fprintf(stderr, "ERROR in GIPSVideoEngine::Delete\n");
    }
}

void
VideoSourceGIPS::Start()
{
    int error = 0;

    frames = 0;

    error = ptrViECapture->ConnectCaptureDevice(gipsCaptureId,
        videoChannel);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViECapture::GIPSViE_ConnectCaptureDevice\n");
        return;
    }

    error = ptrViECapture->StartCapture(gipsCaptureId, cap);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViECapture::GIPSViE_StartCapture\n");
        return;
    }

    // Setup external renderer
    ptrViERender = webrtc::ViERender::GetInterface(ptrViE);
    if (ptrViERender == NULL) {
        fprintf(stderr, "ERROR in GIPSViERender::GIPSViE_GetInterface\n");
        return;
    }

    error = ptrViERender->AddRenderer(gipsCaptureId, webrtc::kVideoI420, this);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViERender::GIPSViE_AddRenderer\n");
        return;
    }

    error = ptrViERender->StartRender(gipsCaptureId);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViERender::GIPSViE_StartRender\n");
        return;
    }

    return;
}

void
VideoSourceGIPS::Stop()
{
    int error = 0;

    error = ptrViERender->StopRender(gipsCaptureId);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViERender::GIPSViE_StopRender\n");
        return;
    }

    error = ptrViERender->RemoveRenderer(gipsCaptureId);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViERender::GIPSViE_RemoveRenderer\n");
        return;
    }

    error = ptrViECapture->StopCapture(gipsCaptureId);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViECapture::GIPSViE_StopCapture\n");
        return;
    }

    error = ptrViECapture->DisconnectCaptureDevice(videoChannel);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSViECapture::GIPSViE_DisconnectCaptureDevice\n");
        return;
    }

    ptrViERender->Release();
    ptrViECapture->Release();

    fprintf(stderr, "\nCaptured %d frames.\n", frames);

    return;
}

int
VideoSourceGIPS::FrameSizeChange(
    unsigned int width, unsigned int height, unsigned int numberOfStreams)
{
    // XXX: Hmm?
    fprintf(stderr, "\nGot FrameSizeChange: %d %d\n", width, height);
    return -1;
}

int
VideoSourceGIPS::DeliverFrame(unsigned char* buffer, int bufferSize)
{
    fwrite("FRAME\n", 6, 1, tmp);
    fwrite(buffer, bufferSize, 1, tmp);
    frames++;

    fprintf(stderr, "Video frame %08d\n", frames);
    return 0;
}


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
    webrtc::VoEHardware *ptrVoEHardware;
    webrtc::VoEExternalMedia* ptrVoERender;
    webrtc::CodecInst codec;
    int channel;
};

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

    ptrVoEBase->SetSendDestination(channel, 8000, "127.0.0.1");
    ptrVoEBase->SetLocalReceiver(channel, 8000);

    strcpy(codec.plname, "PCMU");
    codec.channels = 1;
    codec.pacsize = 160;
    codec.plfreq = 8000;
    codec.pltype = 0;
    codec.rate = 64000;

}

AudioSourceGIPS::~AudioSourceGIPS()
{
    int error;

    error = ptrVoEBase->DeleteChannel(channel);
    if (error == -1) {
        fprintf(stderr, "ERROR in GIPSVoEBase::DeleteChannel\n");
    }

    ptrVoEBase->Terminate();
    ptrVoEBase->Release();
    ptrVoEHardware->Release();
    ptrVoERender->Release();

    if (webrtc::VoiceEngine::Delete(ptrVoE) == false) {
        fprintf(stderr, "ERROR in GIPSVoiceEngine::Delete\n");
    }
}

void
AudioSourceGIPS::Start()
{
    int error;

    error = ptrVoEBase->StartReceive(channel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEBase::StartReceive\n");
    }
    error = ptrVoEBase->StartPlayout(channel);
    if (error) {
        fprintf(stderr, "ERROR in GIPSVoEBase::StartPlayout\n");
    }
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
}

void AudioSourceGIPS::Process(const int channel,
             const webrtc::ProcessingTypes type,
             WebRtc_Word16 audio10ms[], const int length,
             const int samplingFreq, const bool isStereo)
{
    fprintf(stderr, "Audio frame %08d buffer %08llx\t %d\t %d Hz %s\n",
            frames,
            (unsigned long long)audio10ms, length, samplingFreq,
            isStereo ? "stereo" : "mono");
    frames++;
    return;
}

/*** test harness ***/
int
main()
{
    VideoSourceGIPS *video = new VideoSourceGIPS();
    video->Start();

    AudioSourceGIPS *audio = new AudioSourceGIPS();
    audio->Start();

    std::string str;
    std::cout << "Press <enter> to stop... ";
    std::getline(std::cin, str);

    audio->Stop();
    delete audio;

    video->Stop();
    delete video;

    return 0;
}
