//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server/server/connection.hpp"
#include <vector>
#include <boost/bind/bind.hpp>
#include "server/server/connection_manager.hpp"
#include "server/server/request_handler.hpp"
#include <iostream>

namespace http {
namespace server {

connection::connection(boost::asio::io_context& io_context,
    connection_manager& manager, request_handler& handler)
  : socket_(io_context),
    connection_manager_(manager),
    request_handler_(handler)
{
}

boost::asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}

void connection::start()
{
  socket_.async_read_some(boost::asio::buffer(buffer_), //to-do : check here
      boost::bind(&connection::handle_read, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  // boost::asio::async_read(socket_, boost::asio::buffer(buffer_), //to-do : check here
  //     boost::asio::transfer_all(),
  //     boost::bind(&connection::handle_read, shared_from_this(),
  //       boost::asio::placeholders::error,
  //       boost::asio::placeholders::bytes_transferred));

}

void connection::stop()
{
  socket_.close();
}

void connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{

  std::cout<<"Handle read receives "<<bytes_transferred<<std::endl;
  if (!e)
  {
    boost::tribool result;
    boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
        request_, buffer_.data(), buffer_.data() + bytes_transferred);

    if (result)
    {
      std::cout<<"\r\n\r\nbody received"<<std::endl;
      std::cout<<request_.jsonData<<std::endl;
      std::cout<<"end\r\n\r\n"<<std::endl;

      request_handler_.handle_request(request_, reply_);
      boost::asio::async_write(socket_, reply_.to_buffers(),
          boost::bind(&connection::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
    }
    else if (!result)
    {
      reply_ = reply::stock_reply(reply::bad_request);
      boost::asio::async_write(socket_, reply_.to_buffers(),
          boost::bind(&connection::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
    }
    else
    {
      socket_.async_read_some(boost::asio::buffer(buffer_),
          boost::bind(&connection::handle_read, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
  }
  else if (e != boost::asio::error::operation_aborted)
  {
    connection_manager_.stop(shared_from_this());
  }
}

void connection::handle_write(const boost::system::error_code& e)
{
  if (!e)
  {
    // Initiate graceful connection closure.
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }

  if (e != boost::asio::error::operation_aborted)
  {
    connection_manager_.stop(shared_from_this());
  }
}

} // namespace server
} // namespace http