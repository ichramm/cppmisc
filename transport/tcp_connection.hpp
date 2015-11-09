/**
 * \file tcp_connection.hpp
 * \author ichramm
 *
 * Created on
 */
#ifndef transport_tcp_connection_hpp__
#define transport_tcp_connection_hpp__

#include "debug/log.hpp"

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <array>

namespace et {
namespace transport {

namespace asio {
    using boost::asio::async_read;
}

class tcp_connection
    : public std::enable_shared_from_this<tcp_connection>
{
public:
    typedef std::shared_ptr<tcp_connection> ptr;

    typedef boost::system::error_code      error_code;
    typedef std::vector<char>              buffer_type;
    typedef boost::asio::ip::tcp::socket   socket_type;
    typedef boost::asio::ip::tcp::resolver resolver_type;
    typedef boost::asio::ip::tcp::endpoint endpoint_type;

    tcp_connection(boost::asio::io_service& ioservice)
     : socket_(ioservice)
     , resolver_(ioservice)
    { }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    template<typename Connect_Handler>
    void connect(const std::string& host,
                 uint16_t port,
                 BOOST_ASIO_MOVE_ARG(Connect_Handler) callback)
    {
        __TRACE(debug::masks::tcp_trace, "Connecting to %s:%u ..", host.c_str(), port);

        resolver_type::query query(host, boost::lexical_cast<std::string>(port));
        resolver_.async_resolve(query,
                                [this, callback](const error_code& error, resolver_type::iterator it) {
                                    if (error != 0) {
                                        callback(error);
                                    } else {
                                        socket_.async_connect(*it, callback);
                                    }
                                });
    }

    /**
     * \brief Reads data from the socket
     *
     * \param bytes Number of bytes to read
     * \param callback Function to call when done:
     * \code callback(error_code: boost::system::error_code, buffer: std::vector<char>) \endcode
     */
    template<typename Read_Handler>
    void read(size_t bytes,
              BOOST_ASIO_MOVE_ARG(Read_Handler) callback)
    {
        incoming_data_.resize(bytes);
        read(bytes, incoming_data_, [this, callback](const error_code& error) {
            callback(error, std::move(incoming_data_));
        });
    }

    /**
     * \brief Reads data from the socket into the provided buffer
     *
     * \param bytes Number of bytes  to read
     * \param buffer Buffer on which to place read bytes
     * \param callback Function to call when done:
     * \code callback(error_code: boost::system::error_code) \endcode
     *
     * \remarks \p buffer must be long enough to hold \p bytes bytes
     */
    template<typename Buffer_Type,
             typename Read_Handler>
    void read(size_t bytes,
              Buffer_Type& data,
              BOOST_ASIO_MOVE_ARG(Read_Handler) callback)
    {
        __TRACE(debug::masks::tcp_trace, "Asked to read %zu bytes", bytes);
        read(bytes, data, callback, 0);
    }

    /**
     * \brief Writes data to the socket
     *
     * \param data Data to send
     * \param callback Function to call when done
     */
    template<typename Write_Handler>
    void write(std::vector<char> data,
               BOOST_ASIO_MOVE_ARG(Write_Handler) callback)
    {
        __TRACE(debug::masks::tcp_trace, "Asked to write buffer of %zu bytes", data.size());
        if (data.size() < BUFFER_LENGTH) {
            __DUMP_BUFFER(stderr, "Write:", data, data.size());
        }
        outgoing_data_ = std::move(data);
        write(callback, 0);
    }

private:

    static const size_t BUFFER_LENGTH = 1024;

    boost::asio::ip::tcp::socket socket_;
    resolver_type resolver_;

    std::array<char, BUFFER_LENGTH> read_buffer_;
    std::array<char, BUFFER_LENGTH> write_buffer_;

    std::vector<char> incoming_data_;
    std::vector<char> outgoing_data_;


    template<typename Buffer_Type,
             typename Read_Handler>
    void read(size_t bytes,
              Buffer_Type& buffer,
              BOOST_ASIO_MOVE_ARG(Read_Handler) callback,
              size_t read_bytes)
    {
        asio::async_read(socket_,
                         boost::asio::buffer(read_buffer_,
                                             size_t(bytes < BUFFER_LENGTH ? bytes : BUFFER_LENGTH)),
                         [bytes](const error_code& error, size_t len) {
                            return error != 0 || len >= bytes;
                         },
                         [=, &buffer](const error_code& error, size_t len) {
                            if (error != 0) {
                                callback(error);
                            } else {
                                std::memcpy(&buffer[read_bytes], &read_buffer_[0], len);
                                if (bytes-len > 0) {
                                    read(bytes-len, buffer, std::move(callback), read_bytes+len);
                                } else {
                                    if (buffer.size() < BUFFER_LENGTH) {
                                        __DUMP_BUFFER(stderr, "Read:", buffer, buffer.size());
                                    }
                                    callback(boost::system::error_code());
                                }
                            }
                         });
    }

    template<typename Write_Handler>
    void write(BOOST_ASIO_MOVE_ARG(Write_Handler) callback, size_t sent)
    {
        size_t increment = outgoing_data_.size() - sent;
        increment = increment < BUFFER_LENGTH ? increment : BUFFER_LENGTH;
        memcpy(&write_buffer_[0], &outgoing_data_[sent], increment);

        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_buffer_, increment),
                                 [=](const error_code& error,
                                     size_t bytes_transferred) {
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

#endif // transport_tcp_connection_hpp__
