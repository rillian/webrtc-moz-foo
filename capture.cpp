/* Test client
   Use our webrtc wrapper classes to capture audio and video to a file.
 */

#include <iostream>
#include <string>

#include "AudioCapture.h"
#include "VideoCapture.h"

/*** test harness ***/

int main()
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
