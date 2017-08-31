#include "audiosdk.h"
//#include "framework/logger/LoggerSection.h"
//#include "framework/logger/LoggerStreamRecord.h"
//using namespace framework::logger;
//
//FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("AudioSdk", 0);

namespace pps{
    namespace audiosdk{
        AudioSdk::AudioSdk(std::string ip, boost::int32_t port)
            : ip_(ip)
            , port_(port)
            , sequence_(0)
            , duration_(0)
            , time_base_(0)
        {
            format_.sample_rate = 44100;
            format_.bits_per_channel = 16;
            format_.channels_per_frame = 2;
            memset(&parament_, 0, sizeof(parament_));
            memset(&aacframe_, 0, sizeof(aacframe_));
            rtp_packets_[0] = (RTP_Packet *)malloc(sizeof(RTP_Packet));
            rtp_packets_[1] = (RTP_Packet *)malloc(sizeof(RTP_Packet));
        }
        AudioSdk::~AudioSdk()
        {

        }
        boost::int16_t AudioSdk::init(boost::int32_t samplerate, boost::int32_t channels, boost::int32_t bits_per_channel)
        {
            boost::int16_t ret;

            format_.sample_type = PCM_TYPE_SINT;
            if (samplerate && channels && bits_per_channel)
            {
                format_.sample_rate = samplerate;
                format_.bits_per_channel = bits_per_channel;
                format_.channels_per_frame = channels;
            }
            format_.bytes_per_frame = format_.channels_per_frame * ((format_.bits_per_channel + 7) / 8);
            format_.channel_mask = 0;
            duration_ = (double)1024 * 1000 / samplerate;
            aacenc_set_parament();
            ret = aacenc_init(&encoder_, &parament_, &format_, &info_);
            if (ret)
            {
                //LOG_SECTION();
                //LOG_S(Logger::kLevelDebug, "aacenc init error!");
				printf("aacenc init error!\n");
            }
            ret = rtp_sender_.start(ip_, port_);
            if (ret)
            {
                /*LOG_S(Logger::kLevelAlarm, "rtp sender start error!");*/
				printf("rtp sender start error!\n");
            }
            return ret;
        }
        boost::int16_t AudioSdk::release()
        {
            rtp_sender_.stop();
            free(rtp_packets_[0]);
            free(rtp_packets_[1]);
            aacEncClose(&encoder_);
            encoder_ = NULL;
            return 0;
        }
        boost::int32_t AudioSdk::audio_encode_send(boost::uint8_t *buffer, boost::int32_t timestamp)
        {
            boost::uint16_t  got_rtp = 0;
            boost::uint32_t  rtp_size;
            boost::int32_t  samples;
            boost::int32_t  ret;
            boost::uint8_t  aac[1024] = {0};

            if (!aac)
            {
                return -1;
            }
            samples = aac_encode_frame(encoder_, &format_, (const int16_t *)buffer, 1024, &aacframe_);
            if (samples < 0 || (!aacframe_.size && !samples))
            {
                printf("samples:%d, frame size:%d\n", samples, aacframe_.size);
                return -1;
            }
            if (samples && !aacframe_.size)
            {
                return 0;
            }
#if 0
            static FILE *ofp = fopen("audiosdk.aac", "wb");
            memcpy(aac, aacframe_.data, aacframe_.size);
            ret = fwrite(aac, sizeof(uint8_t), aacframe_.size, ofp);
            if (ret != aacframe_.size)
            {
                //LOG_SECTION();
                //LOG_S(Logger::kLevelAlarm, "write frame length " << ret << " frame size " << aacframe_.size << " errno " << errno);
				printf("write frame length %d frame size %d errno %d\n", ret, aacframe_.size, errno);
                //break;
            }
#endif
            //framesize += aacframe_.size;
            //printf("writen size %d total frame size %d cur frame %d\n", ftell(ofp), framesize, aacframe.size);

            sequence_ ++;
            if (!timestamp)
            {
                timestamp = time_base_ + (uint32_t)((duration_*10 + 4)/10);
                time_base_ = timestamp;
            }
            else
            {
                time_base_ = timestamp;
            }

            rtp_packet_ = aac_wrap_rtp(rtp_packets_, aacframe_.data, aacframe_.size, timestamp, sequence_, &got_rtp, &rtp_size);
            if (got_rtp)
            {
                //TODO send rtp_packet  size rtp_size
                rtp_sender_.write((boost::uint8_t *)rtp_packet_, rtp_size);
            }

            return 0;
        }
        RTP_Packet *AudioSdk::aac_wrap_rtp(RTP_Packet **rtps, boost::uint8_t *buf, boost::uint16_t length, boost::uint32_t timestamp, boost::uint16_t sequence, boost::uint16_t *got_rtp, boost::uint32_t *rtp_size)
        {
            static boost::uint32_t cur_pos = 0;
            static boost::uint16_t count = 0;
            RTP_Packet *rtp = *rtps;

            count ++;
            *got_rtp = 0;
            if (!buf && rtps && !length)
            {
                *rtp_size = cur_pos + sizeof(RTP_Header);
                cur_pos = 0;
                count = 0;
                *got_rtp = 1;
                /*LOG_S(Logger::kLevelDebug, "flush all packets in memory\n");*/
				printf("flush all packets in memory\n");
                return rtp;
            }

            if (cur_pos && length + cur_pos < MAX_RTP_LENGTH && count < 10)
            {
                memcpy(rtp->data + cur_pos, buf, length);
                cur_pos += length;
            }
            else if (!cur_pos && length + cur_pos < MAX_RTP_LENGTH && count < 10)
            {
                memcpy(rtp->data + cur_pos, buf, length);
                cur_pos += length;
                rtp->rtp_header.version = 2;
                rtp->rtp_header.padding = 0;
                rtp->rtp_header.extension = 0;
                rtp->rtp_header.csrc_count = 0;
                rtp->rtp_header.marker = 1;
                rtp->rtp_header.payloadtype = RTP_PAYLOAD_AAC;
                rtp->rtp_header.seq = sequence;
                rtp->rtp_header.timestamp = timestamp;
                rtp->rtp_header.ssrc = RTP_SSRC_AAC;
                rtp->rtp_header.csrc = 0;
                rtp->extension.profile = 0;
                rtp->extension.length = 1;
                rtp->extension.extension = cur_pos;
            }
            else if ((length + cur_pos > MAX_RTP_LENGTH) || count == 10)
            {
                rtp = rtps[1];
                rtp->rtp_header.version = rtps[0]->rtp_header.version;
                rtp->rtp_header.padding = rtps[0]->rtp_header.padding;
                rtp->rtp_header.extension = rtps[0]->rtp_header.extension;
                rtp->rtp_header.ssrc = rtps[0]->rtp_header.ssrc;
                rtp->rtp_header.marker = rtps[0]->rtp_header.marker;
                rtp->rtp_header.payloadtype = rtps[0]->rtp_header.payloadtype;
                rtp->rtp_header.seq = rtps[0]->rtp_header.seq;
                rtp->rtp_header.timestamp = rtps[0]->rtp_header.timestamp;
                rtp->rtp_header.csrc = 0;
                rtp->extension.profile = 0;
                rtp->extension.length = 1;
                rtp->extension.extension = cur_pos;
                memcpy(rtp->data, rtps[0]->data, cur_pos);
                *rtp_size = cur_pos + sizeof(RTP_Header) + sizeof(Header_Extension);
                cur_pos = 0;
                count = 1;
                *got_rtp = 1;

                memcpy(rtps[0]->data + cur_pos, buf, length);
                cur_pos += length;
                rtps[0]->rtp_header.version = 2;
                rtps[0]->rtp_header.padding = 0;
                rtps[0]->rtp_header.extension = 1;
                rtps[0]->rtp_header.csrc_count = 0;
                rtps[0]->rtp_header.marker = 1;
                rtps[0]->rtp_header.payloadtype = RTP_PAYLOAD_AAC;
                rtps[0]->rtp_header.seq = sequence;
                rtps[0]->rtp_header.timestamp = timestamp;
                rtps[0]->rtp_header.ssrc = RTP_SSRC_AAC;
                rtps[0]->rtp_header.csrc = 0;
                rtps[0]->extension.profile = 0;
                rtps[0]->extension.length = 1;
                rtps[0]->extension.extension = cur_pos;
                //LOG_S(Logger::kLevelAlarm, "this is a new rtp packet total " << cur_pos << " frame size " << length << " rtp size " << *rtp_size << " ts " << rtp->rtp_header.timestamp);
				printf("this is a new rtp packet total %d frame size %d rtp size %d ts %d\n", cur_pos, length, *rtp_size, rtp->rtp_header.timestamp);
            }
            //LOG_S(Logger::kLevelAlarm, "cur frame " << length << " total " << cur_pos << " count " << count);
			printf("cur frame %d total %d count %d\n", length, cur_pos, count);
            return rtp;
        }

        void AudioSdk::aacenc_set_parament()
        {
            parament_.bitrate = 32000;
            parament_.profile = AOT_AAC_LC;
            parament_.bitrate_mode = 0;
            parament_.bandwidth = 0;         //AAC-LC only
            parament_.afterburner = 0;
            parament_.adts_crc_check = 1;    //add crc protection on adts header
            parament_.header_period = 0;     //StreamMuxConfig repetition period in transportlay
            parament_.lowdelay_sbr = 1;
            parament_.sbr_ratio = 0;         //0 default,1 downsampled sbr(default for ELD-SBR),2 dual-rate SBR(default for HE)
            parament_.sbr_signaling = 0;
            parament_.transport_format = 2;  //2 adts
            return ;
        }
    }
}

