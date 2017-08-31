#include "audiosdk.h"
#include "Timer.h"
#include "audio_processing.h"
#include "resampler.h"
//#include <framework/configure/Config.h>
//#include <framework/logger/Logger.h>
//#include <framework/logger/LoggerSection.h>
//#include <framework/logger/LoggerStreamRecord.h>
//
//FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("main", 0);
//using namespace framework::configure;
//using namespace framework::logger;
using namespace pps::audiosdk;

#pragma comment(lib,"rtc_base_approved.lib")
#pragma comment(lib,"rtc_event_log_proto.lib")
#pragma comment(lib,"rtc_event_log.lib")
#pragma comment(lib,"rent_a_codec.lib")

#pragma comment(lib,"audio_encoder_interface.lib")
#pragma comment(lib,"audio_decoder_interface.lib")
#pragma comment(lib,"audio_coding_module.lib")
#pragma comment(lib,"audio_conference_mixer.lib")
#pragma comment(lib,"audio_device.lib")
#pragma comment(lib,"audio_processing.lib")
#pragma comment(lib,"audio_processing_sse2.lib")
#pragma comment(lib,"audioproc_debug_proto.lib")
#pragma comment(lib,"protobuf_lite.lib")
#pragma comment(lib,"metrics_default.lib")
#pragma comment(lib,"system_wrappers.lib")
#pragma comment(lib,"common_audio.lib")
#pragma comment(lib,"common_audio_sse2.lib")

void InitAudioProcessing(webrtc::AudioProcessing *apm)
{
	apm->level_estimator()->Enable(true);

	apm->echo_cancellation()->Enable(true);

	apm->echo_cancellation()->enable_metrics(true);

	apm->echo_cancellation()->enable_drift_compensation(true); //为TRUE时必须调用set_stream_drift_samples

	apm->gain_control()->Enable(true);

	apm->high_pass_filter()->Enable(true);

	apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kHigh);

	apm->noise_suppression()->Enable(true);

	apm->voice_detection()->Enable(true);

	apm->voice_detection()->set_likelihood(webrtc::VoiceDetection::kModerateLikelihood);

	apm->Initialize();

}

void main(int argc, char **argv)
{
    boost::int32_t size, ret, length = 0;
    boost::uint8_t buffer[4096] = {0};
	boost::uint8_t dst[4096] = { 0 };
    FILE *ifp, *ofp;
	const float *const *src;
    char *input;
    AudioSdk    audiosdk("10.110.48.109", 6090);
	webrtc::AudioProcessing	*apm = webrtc::AudioProcessing::Create();

    input = argv[1];
    ifp = fopen(input, "rb");
    printf("error %d\n", errno);
    fseek(ifp, 0, SEEK_END);
    size = ftell(ifp);
    fseek(ifp, 0, SEEK_SET);
    boost::system::error_code   ec;
    //boost::asio::io_service     io_service;
    std::string log_file = "audioclinet.log";
	ofp = fopen("webrtc.pcm", "wb");
    //Config config;
    //config.profile().post_set("Logger.stream_count=2");
    //config.profile().post_set("LogStream0.file="+log_file);
    //config.profile().post_set("LogStream0.level=4");
    //config.profile().post_set("LogStream0.daily=1");

    //config.profile().post_set("LogStream1.file=STDOUT");
    //config.profile().post_set("LogStream1.level=4");
    //config.profile().post_set("LogStream1.daily=1");

    //global_logger().load_config(config);
    //LOG_S(Logger::kLevelDebug, "[main] logger start ");
	InitAudioProcessing(apm);

    //ret = audiosdk.init(44100, 2, 16);
    //if (ret)
    //{
    //    return ;
    //}
	TinyPerformanceTimer timer;
    while (1)
    {
        ret = fread(buffer, sizeof(uint8_t), 4096, ifp);
        if (ret < 0)
        {
            printf("error %d\n", ftell(ifp));
            break;
        }
        length += 4096;

		timer.BeginTime();

		//apm->ProcessReverseStream();
		ret = apm->ProcessStream((const float *const *)buffer, 1024, 44100, webrtc::AudioProcessing::ChannelLayout::kStereo, 44100, webrtc::AudioProcessing::ChannelLayout::kStereo, (float *const *)dst);
        //audiosdk.audio_encode_send(buffer, 0);
		fwrite(dst, 1, 4096, ofp);
		timer.EndTime();
		printf("time cost %lld\n", timer.GetMillisconds());
        if (size <= length)
            break;
    }
    //audiosdk.release();
	fclose(ofp);
	if (apm)
		delete apm;
}