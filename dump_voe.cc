/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/* Test and dump an enumeration of voice engine capabilities */

#include <stdio.h>
#include <string.h>

#include <voe_base.h>
#include <voe_codec.h>
#include <voe_hardware.h>

using namespace webrtc;

int main(int argc, char *argv[])
{
  VoiceEngine* voe = VoiceEngine::Create();
  VoEBase* base = VoEBase::GetInterface(voe);
  VoEHardware* device = VoEHardware::GetInterface(voe);
  VoECodec* codec = VoECodec::GetInterface(voe);

  base->Init();

  // Enumerate and dump devices
  int num_devices;
  device->GetNumOfRecordingDevices(num_devices);
  fprintf(stderr, "Voice Engine reports %d recording devices:\n", num_devices);
  for (int i = 0; i < num_devices; i++) {
    const unsigned kMaxLength = 128;
    char name[kMaxLength];
    char guid[kMaxLength];
    memset(name, 0, kMaxLength);
    memset(guid, 0, kMaxLength);
    device->GetRecordingDeviceName(i, name, guid);
    fprintf(stderr, "%d\t%s\n", i, name);
  }

  device->GetNumOfPlayoutDevices(num_devices);
  fprintf(stderr, "Voice Engine reports %d playback devices:\n", num_devices);
  for (int i = 0; i < num_devices; i++) {
    const unsigned kMaxLength = 128;
    char name[kMaxLength];
    char guid[kMaxLength];
    memset(name, 0, kMaxLength);
    memset(guid, 0, kMaxLength);
    device->GetPlayoutDeviceName(i, name, guid);
    fprintf(stderr, "%d\t%s\n", i, name);
  }

  // Enumerate and dump codecs
  int num_codecs = codec->NumOfCodecs();
  fprintf(stderr, "Voice Engine reports %d audio codecs:\n", num_codecs);

  for (int i = 0; i < num_codecs; i++) {
    CodecInst c;

    codec->GetCodec(i, c);
    fprintf(stderr, "%d\t%s\t%d Hz %s\n",
        i, c.plname, c.plfreq, c.channels == 1 ? "mono" : "");
  }

  codec->Release();
  device->Release();
  base->Terminate();
  base->Release();

  return 0;
}
