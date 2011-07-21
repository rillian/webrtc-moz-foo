/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/* Test and dump an enumeration of video engine capabilities */

#include <stdio.h>
#include <string.h>

#include <vie_base.h>
#include <vie_capture.h>
#include <vie_codec.h>

using namespace webrtc;

int main(int argc, char *argv[])
{
  VideoEngine* vie = VideoEngine::Create();
  ViEBase* base = ViEBase::GetInterface(vie);
  ViECapture* device = ViECapture::GetInterface(vie);
  ViECodec* codec = ViECodec::GetInterface(vie);

  base->Init();

  // Enumerate and dump devices
  int num_devices = device->NumberOfCaptureDevices();
  fprintf(stderr, "Video Engine reports %d recording devices:\n", num_devices);
  for (int i = 0; i < num_devices; i++) {
    const unsigned kMaxLength = 256;
    char name[kMaxLength];
    char guid[kMaxLength];
    memset(name, 0, kMaxLength);
    memset(guid, 0, kMaxLength);
    device->GetCaptureDevice(i, name, kMaxLength, guid, kMaxLength);
    fprintf(stderr, "%d\t%s (%s)\n", i, name, guid);
  }

  // Enumerate and dump codecs
  int num_codecs = codec->NumberOfCodecs();
  fprintf(stderr, "Video Engine reports %d audio codecs:\n", num_codecs);

  for (int i = 0; i < num_codecs; i++) {
    VideoCodec c;

    codec->GetCodec(i, c);
    fprintf(stderr, "%d\t%s\n", i, c.plName);
  }

  codec->Release();
  device->Release();
  base->Release();
  base->Release();

  return 0;
}
