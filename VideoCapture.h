/*
   Uses the webrtc capture routines to record video to a file.
 */

#ifndef VideoCaptureGIPS_H
#define VideoCaptureGIPS_H

#include <stdio.h>

#include <vie_base.h>
#include <vie_codec.h>
#include <vie_render.h>
#include <vie_capture.h>

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

#endif // VideoCaptureGIPS_H
