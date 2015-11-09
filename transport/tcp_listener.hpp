/**
 * \file tcp_listener.hpp
 * \author ichramm
 *
 * Created on
 */
#ifndef transport_tcp_listener_hpp__
#define transport_tcp_listener_hpp__

#include "transport/tcp_connection.hpp"
#include <thread>

namespace et {
namespace transport {

/**
 * @brief Accept TCP connections on a given port
 */
class tcp_listener
{
    static const size_t THREADS = 2;
public:

    typedef std::function<void(boost::system::error_code,
                               tcp_connection::ptr)> Handler_Type;

    /**
     * @brief Constructor
     *
     * @param port Bind port
     * @param ip Local address on which to bind to, empty means all interfaces
     */
    tcp_listener(unsigned short port,
                 std::string ip = "")
     : ip_(ip)
     , port_(port)
     , work_(ioservice_)
     , acceptor_(ioservice_)
     , threads_(THREADS)
    { }

    ~tcp_listener() {
        stop();
    }

    void set_threads(int32_t threads)
    {
        threads_.resize(threads);
    }

    template <typename Handler>
    void start(Handler handler)
    {
        connection_handler_ = Handler_Type{std::move(handler)};

        ioservice_.reset(); // allows to start() -> stop() -> start()

        for (auto& thread: threads_) {
            thread = std::thread([this] {
                ioservice_.run();
            });
        }

        boost::asio::ip::tcp::endpoint endpoint;
        if (ip_.empty()) {
            endpoint =  boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_);
        } else {
            endpoint =  boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_), port_);
        }

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        async_accept();
    }

    void stop()
    {
        acceptor_.close();
        ioservice_.stop();
        for (auto& thread : threads_) {
            thread.join();
        }

        connection_handler_ = Handler_Type();
    }

    void async_accept()
    {
        tcp_connection::ptr connection = std::make_shared<tcp_connection>(ioservice_);
        acceptor_.async_accept(connection->socket(), [=](const boost::system::error_code& error) {
            if (error != 0) {
                connection_handler_(error, tcp_connection::ptr());
            } else {
                connection_handler_(std::move(error), std::move(connection));
                async_accept();
            }
        });
    }

protected:
    std::string                    ip_;
    uint16_t                       port_;
    boost::asio::io_service        ioservice_;
    boost::asio::io_service::work  work_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::vector<std::thread>       threads_;
    Handler_Type                   connection_handler_;
};

} // namespace transport
} // namespace et

#endif // transport_tcp_listener_hpp__
