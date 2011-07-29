PROGS := capture dump_voe dump_vie

CPPFLAGS ?= -g -Wall
CPPFLAGS += -I../src
CPPFLAGS += -I../src/voice_engine/main/interface
CPPFLAGS += -I../src/video_engine/main/interface

capture_SRCS := capture.cpp AudioCapture.cpp VideoCapture.cpp
dump_voe_SRCS := dump_voe.cc
dump_vie_SRCS := dump_vie.cc

WEBRTC_BUILDPATH ?= ../out/Debug/obj.target
WEBRTC_LIBS := \
  src/video_engine/main/source/libvideo_engine_core.a \
  src/voice_engine/main/source/libvoice_engine_core.a \
  src/modules/video_render/main/source/libvideo_render_module.a \
  src/modules/media_file/source/libmedia_file.a \
  src/modules/video_processing/main/source/libvideo_processing.a \
  src/modules/video_capture/main/source/libvideo_capture_module.a \
  src/modules/utility/source/libwebrtc_utility.a \
  src/modules/video_coding/main/source/libwebrtc_video_coding.a \
  src/modules/video_coding/codecs/vp8/main/source/libwebrtc_vp8.a \
  src/common_video/jpeg/main/source/libwebrtc_jpeg.a \
  third_party/libvpx/libvpx.a \
  third_party/libjpeg_turbo/libjpeg_turbo.a \
  src/modules/audio_coding/main/source/libaudio_coding_module.a \
  src/modules/audio_processing/main/source/libaudio_processing.a \
  src/modules/audio_device/main/source/libaudio_device.a \
  src/modules/audio_coding/NetEQ/main/source/libNetEq.a \
  src/modules/audio_coding/codecs/opus/main/source/libopus.a \
  src/modules/audio_processing/ns/main/source/libns_fix.a \
  src/modules/audio_processing/aecm/main/source/libaecm.a \
  src/modules/audio_processing/aec/main/source/libaec.a \
  src/common_audio/resampler/main/source/libresampler.a \
  src/modules/audio_coding/codecs/iLBC/main/source/libiLBC.a \
  src/modules/audio_processing/agc/main/source/libagc.a \
  src/modules/audio_coding/codecs/CNG/main/source/libCNG.a \
  src/modules/audio_coding/codecs/iSAC/fix/source/libiSACFix.a \
  src/modules/audio_coding/codecs/iSAC/main/source/libiSAC.a \
  src/common_audio/vad/main/source/libvad.a \
  src/modules/udp_transport/source/libudp_transport.a \
  src/modules/audio_conference_mixer/source/libaudio_conference_mixer.a \
  src/modules/rtp_rtcp/source/librtp_rtcp.a \
  src/modules/video_coding/codecs/i420/main/source/libwebrtc_i420.a \
  src/common_video/vplib/main/source/libwebrtc_vplib.a \
  third_party/opus/libopus.a \
  src/modules/audio_coding/codecs/G711/main/source/libG711.a \
  src/modules/audio_processing/utility/libapm_util.a \
  src/common_audio/signal_processing_library/main/source/libspl.a \
  src/modules/audio_coding/codecs/G722/main/source/libG722.a \
  src/modules/audio_coding/codecs/PCM16B/main/source/libPCM16B.a \
  src/system_wrappers/source/libsystem_wrappers.a
LIBS := $(WEBRTC_LIBS:%=$(WEBRTC_BUILDPATH)/%) -lrt -lXext -lX11 -lasound -lpulse -ldl -pthread


## boilerplate

all: $(PROGS)

check: all
	./dump_voe
	./dump_vie

clean:
	$(RM) $(ALL_OBJS)
	$(RM) $(PROGS)

dist:
	@echo "not implemented"

.PHONEY: all check clean dist

define PROG_TEMPLATE
$(1)_OBJS := $$($(1)_SRCS:.cc=.o)
$(1)_OBJS := $$($(1)_OBJS:.cpp=.o)
ALL_OBJS += $$($(1)_OBJS)
$(1).o: CPPFLAGS += $$($(1)_CFLAGS)
$(1): $$($(1)_OBJS) $$(filter %.a, $$(LIBS))
	g++ $$(CFLAGS) -o $$@ $$^ $$(LIBS)
endef

$(foreach prog,$(PROGS),$(eval $(call PROG_TEMPLATE,$(prog))))
