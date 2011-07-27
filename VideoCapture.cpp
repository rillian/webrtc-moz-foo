/*
   Uses the webrtc capture routines to record video to a file.
 */

#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>

#include "VideoCapture.h"

/*** implementation ***/

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
