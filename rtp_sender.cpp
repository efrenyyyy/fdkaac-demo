#include <boost/bind.hpp>
#include "rtp_sender.h"
//#include "framework/logger/LoggerSection.h"
//#include "framework/logger/LoggerStreamRecord.h"
#include "audiosdk.h"
//using namespace framework::logger;
//
//FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("RTP_SENDER", 0);

namespace pps{
    namespace audiosdk{
        
        RTP_Sender::RTP_Sender()
            : socket_(io_svr_)
        {
            
        }

        RTP_Sender::~RTP_Sender()
        {

        }

        int RTP_Sender::start(std::string ip, boost::int32_t port)
        {
            end_point_.address(boost::asio::ip::address::from_string(ip));
            end_point_.port(port);
            connect();
            if (ec_)
            {
                return -1;
            }
            return 0;
        }

		void RTP_Sender::callback()
		{

		}

        int RTP_Sender::write(boost::uint8_t *buf, boost::uint16_t len)
        {
            ec_.clear();
            if (!buf || !len)
            {
                return 0;
            }
            RTP_Header *rtp = (RTP_Header *)buf;
            if (rtp->payloadtype != RTP_PAYLOAD_AAC)
            {
                //LOG_S(Logger::kLevelAlarm, "[write] write rtp packet type is " << rtp->payloadtype);
            }
            //socket_.write_some(boost::asio::buffer(buf, len), ec_);
			boost::asio::async_write(socket_, boost::asio::buffer(buf, len), boost::bind(&RTP_Sender::callback, this));
            if (ec_)
            {
                //LOG_S(Logger::kLevelAlarm, "[write] write rtp packet error code " << ec_.message());
				printf("write rtp packet error code %s\n", ec_.message().c_str());
                return ec_.value();
            }
            return 0;
        }

        void RTP_Sender::connect()
        {
			ec_.clear();
            socket_.connect(end_point_, ec_);
            if (ec_)
            {
                //LOG_S(Logger::kLevelAlarm, "[connect] tcp connect error code " << ec_.message());
				printf("tcp connect error code %s\n", ec_.message().c_str());
                return;
            }
            //LOG_SECTION();
            //LOG_S(Logger::kLevelError, "[connect] tcp io connect success!");
			printf("tcp io connect success!\n");
        }

        void RTP_Sender::stop()
        {
            socket_.close();
        }
    }
}
