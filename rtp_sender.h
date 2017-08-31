#ifndef _RTP_SENDER_H_
#define _RTP_SENDER_H_
#include <string.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace pps{
    namespace audiosdk{
        class RTP_Sender
        {
        public:
            RTP_Sender();
            ~RTP_Sender();
            int start(std::string ip, boost::int32_t port);
            int write(boost::uint8_t *buf, boost::uint16_t len);
            void stop();
        protected:
			void callback();
            void connect();
        private:
            boost::asio::io_service         io_svr_;
            boost::asio::ip::tcp::endpoint  end_point_;
            boost::asio::ip::tcp::socket    socket_;
            boost::system::error_code       ec_;
        };
    }
}
#endif
