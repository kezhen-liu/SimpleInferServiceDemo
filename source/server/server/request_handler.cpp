//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server/server/request_handler.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "server/server/mime_types.hpp"
#include "server/server/reply.hpp"
#include "server/server/request.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <stdlib.h>
#include <thread>
#include <memory>

#include <server/PersonPipeline/PersonPipeline.hpp>
#include <common/utility/base64.h>
#include <server/database/historyStorage.hpp>

namespace http {
namespace server {

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
}

void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }
  std::cout<<"Received HTTP method: "<< req.method << std::endl;
  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  if(request_path == "/predict"){
    // in case of predict route
    std::string boundary = "";

    // as the http request transmit multiple images in form of multipart message
    //  we need to find out what the boundary is
    for(const auto& iter : req.headers){
      if(iter.name == "Content-Type"){
        if(!retrieveMultipartBoundary(iter.value, boundary)){
          rep = reply::stock_reply(reply::bad_request);
          return;
        }
      }
    }
    if(boundary==""){
      rep = reply::stock_reply(reply::bad_request);
      return;
    }

    // with known boundary, we extract the image data encoded in base64
    std::unordered_map<std::string, std::string> images;
    if(!retrieveImages(boundary, req.jsonData, images)){
      rep = reply::stock_reply(reply::bad_request);
      return;
    }

    // start and init the person pipeline, which consists of a detection 
    //  network and a person attribute classification network
    SISD::PersonPipeline person;
    person.init();

    std::vector<std::string> results;
    std::stringstream replyData;
    for(const auto& iter : images){
      // std::cout<<"Filename: "<<iter.first<<std::endl;
      // std::cout<<"Image data: "<<std::endl;
      // std::cout<<iter.second<<std::endl;

      std::string base64(iter.second);
      base64.shrink_to_fit();

      // decode base64 to raw binary in form of its original image format
      std::string decoded = base64_decode(base64, true);

      // run the pipeline
      results.push_back(person.run(decoded.data(), decoded.length(), iter.first));
    }
    // we combine results from batch of images together to a single string
    combineJsonResults(results, replyData);

    std::string stringReplyData = replyData.str();

    // generate a unique storage handle and save to history storage
    SISD::HistoryStorage::JobHandle handle = SISD::HistoryStorage::getInstance().generateHandle();
    SISD::HistoryStorage::getInstance().save(handle, stringReplyData);

    // make reply
    rep.content.append(stringReplyData);
    
    rep.status = reply::ok;
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = mime_types::extension_to_type("json");
  }
  else if(request_path == "/history"){
    // in case of a history route

    // we retrieve all records in database
    std::unordered_map<std::string, std::string> history;
    SISD::HistoryStorage::getInstance().getAll(history);

    // make reply
    std::stringstream replyData;
    for(const auto& iter : history){
      replyData << iter.second;
    }
    rep.content.append(replyData.str());

    rep.status = reply::ok;
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = mime_types::extension_to_type("json");

  }
  else{
    // invalid route
    rep = reply::stock_reply(reply::bad_request);
    return;
  }
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

bool request_handler::retrieveMultipartBoundary(const std::string& in, std::string& out){
  out.clear();
  out.reserve(in.size());

  std::string::size_type startIdx = in.find("boundary=");
  if(startIdx == std::string::npos){
    return false;
  }
  startIdx += 9;

  std::string::size_type endIdx = in.find_first_of("/r ;", startIdx);
  if(endIdx == std::string::npos){
    endIdx = in.length();
  }
  out = in.substr(startIdx, endIdx-startIdx+1);
  std::cout << "Found boundary: "<<out<<std::endl;
  return true;
}

bool request_handler::retrieveImages(const std::string& boundary, const std::string& in, std::unordered_map<std::string, std::string>& out){
  out.clear();
  out.reserve(in.size());

  const std::string boundary_rn = "--" + boundary + "\r\n";
  const std::string boundary_end_rn = "--" + boundary + "--\r\n";

  std::string::size_type currentIdx = in.find(boundary_rn, 0);
  if(currentIdx == std::string::npos){
    return false;
  }
  currentIdx += boundary_rn.length();

  // find the next boundary
  std::string::size_type partEndIdx = in.find(boundary_rn, currentIdx);
  while(partEndIdx != std::string::npos){
    std::string part = in.substr(currentIdx, partEndIdx-currentIdx+1);

    // find filename and content here
    std::string filename;
    if(!retrieveFilenameFromMultiPartMessage(part, filename)){
      return false;
    }

    std::string base64image;
    if(!retrieveImageFromMultiPartMessage(part, base64image)){
      return false;
    }
    out[filename] = base64image;

    currentIdx = partEndIdx + boundary_rn.length();
    partEndIdx = in.find(boundary_rn, currentIdx);
  }

  // no boundary_rn any more, try find the boundary_end_rn
  partEndIdx = in.find(boundary_end_rn, currentIdx);
  std::string lastPart = in.substr(currentIdx, partEndIdx-currentIdx+1);

  // find filename and content here
  std::string filename;
  if(!retrieveFilenameFromMultiPartMessage(lastPart, filename)){
    return false;
  }

  std::string base64image;
  if(!retrieveImageFromMultiPartMessage(lastPart, base64image)){
    return false;
  }
  out[filename] = base64image;

  return true;

}

bool request_handler::retrieveFilenameFromMultiPartMessage(const std::string& in, std::string& out){
  out.clear();
  out.reserve(in.size());

  std::string::size_type startIdx = in.find("filename=\"", 0);
  if(startIdx == std::string::npos){
    return false;
  }
  startIdx += 10;
  std::string::size_type endIdx = in.find("\"", startIdx);
  if(endIdx == std::string::npos){
    return false;
  }
  out = in.substr(startIdx, endIdx-startIdx);
  return true;
}

bool request_handler::retrieveImageFromMultiPartMessage(const std::string& in, std::string& out){
  out.clear();
  out.reserve(in.size());

  std::string::size_type startIdx = in.find("\r\n\r\n", 0);
  if(startIdx == std::string::npos){
    return false;
  }
  startIdx += 4;
  std::string::size_type endIdx = in.find("\r\n", startIdx);
  if(endIdx == std::string::npos){
    return false;
  }
  out = in.substr(startIdx, endIdx-startIdx);
  return true;
}

bool request_handler::combineJsonResults(const std::vector<std::string>& results, std::stringstream& resultss){
  resultss<<"{\n";
  for(const auto& iter : results){
    resultss << iter.substr(2, iter.length()-4);
  }
  resultss<<"}\n";
  return true;
}

} // namespace server
} // namespace http
