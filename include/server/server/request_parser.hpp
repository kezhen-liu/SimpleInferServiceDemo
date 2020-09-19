//
// request_parser.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <iostream>
#include "request.hpp"

namespace http {
namespace server {

struct request;

/// Parser for incoming requests.
class request_parser
{
public:
  /// Construct ready to parse the request method.
  request_parser();

  /// Reset to initial parser state.
  void reset();

  /// Parse some data. The tribool return value is true when a complete request
  /// has been parsed, false if the data is invalid, indeterminate when more
  /// data is required. The InputIterator return value indicates how much of the
  /// input has been consumed.
  template <typename InputIterator>
  boost::tuple<boost::tribool, InputIterator> parse(request& req,
      InputIterator begin, InputIterator end)
  {
    // std::string receivedData = begin;
    // std::size_t jsonDataBegin = receivedData.find_first_of("{");
    // std::size_t jsonDataEnd = receivedData.find_last_of("}");
    // req.jsonData = receivedData.substr(jsonDataBegin,(jsonDataEnd - jsonDataBegin + 1));

    // while (begin != end)
    // {
    //   boost::tribool result = consume(req, *begin++);
    //   if (result || !result)
    //     return boost::make_tuple(result, begin);
    // }
    // boost::tribool result = boost::indeterminate;
    // return boost::make_tuple(result, begin);

    // std::string receivedData = begin;

    // std::cout<<"Received length "<<end-begin<<std::endl;
    // std::cout<<"\r\n\r\ndata received"<<std::endl;
    // std::cout<<receivedData<<std::endl;
    // std::cout<<"end\r\n\r\n"<<std::endl;

    // std::size_t jsonDataBegin = receivedData.find_first_of("\r\n\r\n")+4;
    // std::size_t jsonDataEnd = receivedData.find_last_of("\r\n")+2;
    // req.jsonData = receivedData.substr(jsonDataBegin,(jsonDataEnd - jsonDataBegin));

    // end -= jsonDataEnd - jsonDataBegin;
    while (begin != end)
    {
      boost::tribool result = consume(req, *begin++);
      if (result || !result)
        return boost::make_tuple(result, begin);
    }
    boost::tribool result = boost::indeterminate;
    return boost::make_tuple(result, begin);
  }

private:
  /// Handle the next character of input.
  boost::tribool consume(request& req, char input);

  /// Check if a byte is an HTTP character.
  static bool is_char(int c);

  /// Check if a byte is an HTTP control character.
  static bool is_ctl(int c);

  /// Check if a byte is defined as an HTTP tspecial character.
  static bool is_tspecial(int c);

  /// Check if a byte is a digit.
  static bool is_digit(int c);

  /// The current state of the parser.
  enum state
  {
    method_start,
    method,
    uri,
    http_version_h,
    http_version_t_1,
    http_version_t_2,
    http_version_p,
    http_version_slash,
    http_version_major_start,
    http_version_major,
    http_version_minor_start,
    http_version_minor,
    expecting_newline_1,
    header_line_start,
    header_lws,
    header_name,
    space_before_header_value,
    header_value,
    expecting_newline_2,
    expecting_newline_3,
    body_content_normal, //kl: added
    body_content_expect_newline
  } state_;

  std::size_t contentLength;
  std::size_t expectedContentLength;
  std::size_t contentCtr;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_PARSER_HPP