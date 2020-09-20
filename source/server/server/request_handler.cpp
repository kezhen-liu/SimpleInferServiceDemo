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

// namespace AiInferenceServer
// {
//   typedef int Status_t;
//   Status_t Run(std::vector<std::string>& mediaUri, std::string& pipelineConfig, void*& result, size_t& resultSize)
//   {
//     std::cout << "Calling the pipeline manager !!!! " << std::endl;
//     PipelineManager pipMgr;
//     std::shared_ptr<std::thread> sp_pipelineThread(new std::thread(&PipelineManager::runPipeline, &pipMgr));
//     sp_pipelineThread.get()->join();

//     std::vector<FrameData> resultDataVector = pipMgr.getData();
//     int resultDataVectorSize = resultDataVector.size();
//     int resultFeatureSize = resultDataVector[0].frameFeature.size();
//     resultSize = resultDataVectorSize * resultFeatureSize * 4;
//     float * buffer  =  (float*)malloc(resultSize);
//     if (buffer == NULL)
//         exit (1);

//     std::cout<< "total feature number: " << resultDataVectorSize << " feature size: "<< resultFeatureSize << " Buffer Size : "<< resultSize << std::endl;
    
//     for(unsigned dataIndex(0); dataIndex < resultDataVectorSize; ++dataIndex ){
//         FrameData tmpFrameData = resultDataVector[dataIndex];

//         int featureSize =  tmpFrameData.frameFeature.size();
//         for(unsigned i =0; i< featureSize; ++i){
//             const auto& frameFeature = tmpFrameData.frameFeature;
//             buffer[dataIndex * resultFeatureSize + i] = frameFeature[i];
//         }
//     }
//     resultDataVector.clear();

//     result = static_cast<void*>(buffer);
//     return 0;
//   }

// }

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
      || request_path.find("..") != std::string::npos || request_path != "/predict")
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  std::string boundary = "";
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

  std::unordered_map<std::string, std::string> images;
  if(!retrieveImages(boundary, req.jsonData, images)){
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  for(const auto& iter : images){
    std::cout<<"Filename: "<<iter.first<<std::endl;
    std::cout<<"Image data: "<<std::endl;
    std::cout<<iter.second<<std::endl;
  }

  // std::cout<<"\r\n\r\n"<<std::endl;
  // std::cout<<req.jsonData<<std::endl;

  // boost::property_tree::ptree rootReceive;
  // std::stringstream receiveData(req.jsonData);
  
  // try{
  //   boost::property_tree::read_json(receiveData, rootReceive);
  // }
  // catch(std::exception&e)
  // {
  //   std::cout<<"Failed to parse json from string"<<std::endl;
  // }
 
  // std::string functionName = rootReceive.get<std::string>("FunctionName");
  // if(functionName.compare("Run") == 0)
  // {
  //   std::cout<<"Server : extract parameters and call function:"<<functionName<<std::endl;
  //   std::vector<std::string> mediaUris;
  //   for(boost::property_tree::ptree::value_type& mediaUri : rootReceive.get_child("Arguments.In.mediaUri"))
  //   {
  //     mediaUris.push_back(mediaUri.second.data());
  //   }
  //   std::string pipelineConfig = rootReceive.get<std::string>("Arguments.In.pipelineConfig");

  //   void* outputResult;
  //   size_t outputResultSize;
  //   int ret = AiInferenceServer::Run(mediaUris, pipelineConfig, outputResult, outputResultSize);
    
  //   float* floatBuffer = static_cast<float*>(outputResult);
  //   size_t dataNum = outputResultSize/4;
  //   boost::property_tree::ptree resultDataNode;
  //   for (unsigned dataIndex=0; dataIndex < dataNum; ++dataIndex)
  //   {
  //     std::string s = std::to_string(floatBuffer[dataIndex]);
  //     boost::property_tree::ptree singleDataNode;
  //     singleDataNode.put("", s);
  //     resultDataNode.push_back(std::make_pair("", singleDataNode));
  //   }

  //   free(outputResult);
    
  //   rootReceive.add_child("Arguments.Out.resultData", resultDataNode);
  //   rootReceive.put<int>("Arguments.Out.resultDataSize", outputResultSize);
  //   rootReceive.put<int>("Arguments.Return.Status_t", ret);
  // }

  //It should be can the related function and fill data into json and send back to client.
  std::stringstream replyData;
  // boost::property_tree::write_json(replyData, rootReceive);
  replyData << "Test response";
  std::string stringReplyData = replyData.str();
  // rep.content.append("jsonDataBegin:\n");
  rep.content.append(stringReplyData);
  // rep.content.append("jsonDataEnd.");
  
  rep.status = reply::ok;
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type("json");
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
  out = in.substr(startIdx, endIdx-startIdx+1);
  return true;
}

} // namespace server
} // namespace http
