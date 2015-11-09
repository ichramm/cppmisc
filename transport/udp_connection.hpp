/**
 * \file udp_connection.hpp
 * \author ichramm
 *
 * Created on
 */
#ifndef transport_udp_connection_hpp__
#define transport_udp_connection_hpp__

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <array>
#include <memory>

namespace et {
namespace transport {

class udp_connection
    : public boost::enable_shared_from_this<udp_connection>
{
public:
    typedef boost::shared_ptr<udp_connection> ptr;

    typedef boost::system::error_code      error_code;
    typedef std::vector<char>              buffer_type;
    typedef boost::asio::ip::udp::socket   socket_type;
    typedef boost::asio::ip::udp::resolver resolver_type;
    typedef boost::asio::ip::udp::endpoint endpoint_type;


public:

    udp_connection(boost::asio::io_service& ioservice)
     : socket_(ioservice)
     , resolver_(ioservice)
    { }

    socket_type& socket()
    {
        return socket_;
    }

    template <
        typename Connect_Handler>
    void connect(const std::string& host,
                 uint16_t port,
                 BOOST_ASIO_MOVE_ARG(Connect_Handler) callback)
    {
        resolver_type::query query(host, boost::lexical_cast<std::string>(port));
        resolver_.async_resolve(query,
                                [this, callback](const error_code& error, resolver_type::iterator it) {
                                    if (error != 0) {
                                        callback(error);
                                    } else {
                                        /* From connect's man page: http://linux.die.net/man/3/connect
                                         * If the initiating socket is not connection-mode, then connect() sets
                                         * the socket's peer address, but no connection is made. For
                                         * SOCK_DGRAM sockets, the peer address identifies where all
                                         * datagrams are sent on subsequent send() calls, and limits the
                                         * remote sender for subsequent recv() calls.
                                         */
                                        socket_.async_connect(*it, callback);
                                    }
                                });
    }

    /**
     * \brief Reads data from the socket
     *
     * \param callback(error_code, endpoint, std::vector<char>) Function to call when done
     */
    template <
        typename Read_Handler>
    void read(Read_Handler callback)
    {
        socket_.async_receive(boost::asio::null_buffers(), [=](const error_code& error) {
            if (error != 0) {
                callback(error, endpoint_type(), buffer_type());
            } else {
                std::shared_ptr<udp_read_cb_data> read_data = std::make_shared<udp_read_cb_data>(socket_.available());
                socket_.async_receive_from(boost::asio::buffer(read_data->buffer, read_data->buffer.size()),
                                           read_data->endpoint,
                                           [=](const error_code& error, size_t bytes_transferred) {
                                               if (!error) {
                                                   read_data->buffer.resize(bytes_transferred);
                                               }
                                               callback(error, read_data->endpoint, read_data->buffer);
                                           });
            }
        });
    }

    /**
     * \brief Writes data to the socket
     *
     * \param data Data to send
     * \param callback Function to call when done
     */
    template <
        typename Write_Handler>
    void write(std::vector<char> data,
               Write_Handler callback)
    {
        outgoing_data_ = std::move(data);
        write(callback, 0);
    }

private:

    struct udp_read_cb_data {
        buffer_type   buffer;
        endpoint_type endpoint;

        udp_read_cb_data(uint64_t bytes_to_read)
         : buffer(bytes_to_read)
        { }
    };

    static const uint64_t BUFFER_LENGTH = 1024;

    socket_type   socket_;
    resolver_type resolver_;

    std::array<char, BUFFER_LENGTH> write_buffer_;
    std::vector<char> outgoing_data_;


    template <
        typename Write_Handler>
    void write(Write_Handler callback, uint64_t sent)
    {
        size_t increment = outgoing_data_.size() - sent;
        increment = increment < BUFFER_LENGTH ? increment : BUFFER_LENGTH;
        memcpy(&write_buffer_[0], &outgoing_data_[sent], increment);

        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_buffer_, increment),
                                 [=](const error_code& error, size_t bytes_transferred) {
            if (error != 0 || sent+bytes_transferred == outgoing_data_.size()) {
                callback(error);
            } else {
                write(callback, sent + bytes_transferred);
            }
        });
    }
};

} // namespace transport
} // namespace et

#endif // transport_udp_connection_hpp__
