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

#include <voe_base.h>
#include <voe_codec.h>

using namespace webrtc;

int main(int argc, char *argv[])
{
  VoiceEngine* voe = VoiceEngine::Create();
  VoEBase* base = VoEBase::GetInterface(voe);
  VoECodec* codec = VoECodec::GetInterface(voe);

  base->Init();
  int num_codecs = codec->NumOfCodecs();
  fprintf(stderr, "Voice Engine reports %d audio codecs:\n", num_codecs);

  for (int i = 0; i < num_codecs; i++) {
    CodecInst c;

    codec->GetCodec(i, c);
    fprintf(stderr, "%d\t%s\t%d Hz %s\n",
        i, c.plname, c.plfreq, c.channels == 1 ? "mono" : "");
  }

  base->Terminate();
  base->Release();
  codec->Release();

  return 0;
}
