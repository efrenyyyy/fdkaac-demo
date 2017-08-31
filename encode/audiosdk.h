#ifndef AUDIO_SDK_H_
#define AUDIO_SDK_H_
#include "rtp_sender.h"
#include "aacenc_encoder.h"

#define MAX_RTP_LENGTH 1450

namespace pps{
    namespace audiosdk{
        enum eSSRC{
            RTP_SSRC_AAC    =   0x4fabc890,
            RTP_SSRC_PCM    =   0x4fabc891,
        };

        enum ePayload{
            RTP_PAYLOAD_AAC =   97,
            RTP_PAYLOAD_PCM =   98,
        };

        typedef struct RTP_Header_t 
        { 
            boost::uint16_t csrc_count:4; 
            boost::uint16_t extension:1; 
            boost::uint16_t padding:1; 
            boost::uint16_t version:2; 
            boost::uint16_t payloadtype:7; 
            boost::uint16_t marker:1; 
            boost::uint16_t seq; 
            boost::uint32_t timestamp; 
            boost::uint32_t ssrc;
            boost::uint32_t csrc;
        }RTP_Header;

        typedef struct RTP_Header_Extension_t{
            boost::uint16_t profile;
            boost::uint16_t length;
            boost::uint32_t extension;
        }Header_Extension;

        typedef struct RTP_Packet_t
        {
            RTP_Header          rtp_header;
            Header_Extension    extension;
            boost::uint8_t      data[MAX_RTP_LENGTH];
        }RTP_Packet;

        class AudioSdk
        {
        public:
            AudioSdk(std::string ip, boost::int32_t port);
            ~AudioSdk();
            boost::int16_t  init(boost::int32_t samplerate, boost::int32_t channels, boost::int32_t bits_per_channel);
            boost::int16_t  release();
            boost::int32_t  audio_encode_send(boost::uint8_t *buffer, boost::int32_t timestamp);
        protected:
        private:
            RTP_Packet *aac_wrap_rtp(RTP_Packet **rtps, boost::uint8_t *buf, boost::uint16_t length, boost::uint32_t timestamp, boost::uint16_t sequence, boost::uint16_t *got_rtp, boost::uint32_t *rtp_size);
            void aacenc_set_parament();

        private:
            HANDLE_AACENCODER           encoder_;
            AACENC_InfoStruct           info_;
            aacenc_param_t              parament_;
            pcm_sample_description_t    format_;
            aacenc_frame_t              aacframe_;
            RTP_Packet                  *rtp_packets_[2];
            RTP_Packet                  *rtp_packet_;
            boost::int32_t              time_base_;
            uint16_t                    sequence_;
            double                      duration_;
            std::string                 ip_;
            boost::int32_t              port_;
            RTP_Sender                  rtp_sender_;
        };
    }
}

#endif
