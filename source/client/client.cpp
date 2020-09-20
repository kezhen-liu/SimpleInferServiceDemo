#include <iostream>
#include <sstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#include <client/client.hpp>
#include <common/utility/base64.h>

namespace SISD{

class Client::Request::Impl{
public:
    Impl(const FilepathVec& filePath, const RequestType& type);

    Impl();

    operator bool() const;

    ~Impl();

    const FilepathVec& filePath() const;

    RequestType type() const;

private:
    bool m_valid;
    FilepathVec m_filenames;
    RequestType m_type;
};

Client::Request::Impl::Impl(const FilepathVec& filePath, const RequestType& type): m_valid(true),
        m_filenames(filePath), m_type(type){

}

Client::Request::Impl::Impl():m_valid(false), m_filenames({}), m_type(RequestType::Invalid){

}

Client::Request::Impl::operator bool() const{
    return m_valid;
}

Client::Request::Impl::~Impl(){

}

const Client::FilepathVec& Client::Request::Impl::filePath() const{
    return m_filenames;
}

Client::RequestType Client::Request::Impl::type() const{
    return m_type;
}

Client::Request::Request(Request&& req){
    m_impl = std::move(req.m_impl);
}

Client::Request& Client::Request::operator=(Request&& req){
    if(this != &req){
        m_impl = std::move(req.m_impl);
    }
    return *this;
}

Client::Request::operator bool() const{
    return bool(*m_impl);
}

Client::Request::~Request(){

}

const Client::FilepathVec& Client::Request::filePath() const{
    return m_impl->filePath();
}

Client::RequestType Client::Request::type() const{
    return m_impl->type();
}

Client::Request::Request(const FilepathVec& filePath, const RequestType& type){
    m_impl = std::unique_ptr<Impl>(new Impl(filePath, type));
}


class Client::Impl{
public:

    Impl();

    ~Impl();

    Request formRequest(const FilepathVec& filePath, const RequestType& type);

    std::string sendRequest(const Request& req);

private:
    // std::string base64Encode(const char* data, std::size_t size) const;

};

Client::Impl::Impl(){

}

Client::Impl::~Impl(){

}

Client::Request Client::Impl::formRequest(const FilepathVec& filePath, const RequestType& type){
    if(type == Client::Person){
        if(filePath.empty()){
            std::cerr << "File path is empty!" << std::endl;
            return Request();
        }

        bool valid = true;
        for(const auto& file: filePath){
            boost::filesystem::path fpath(file);
            try{
                if(boost::filesystem::exists(fpath)){
                    if (!boost::filesystem::is_regular_file(fpath)){
                        std::cerr << "File " << fpath.string() << "is not a regular file!" << std::endl;
                        valid = false;
                        break;
                    }
                }
                else{
                    std::cerr << "Path " << fpath.string() << " does not exist"<< std::endl;
                    valid = false;
                    break;
                }
            }
            catch (const boost::filesystem::filesystem_error& ex)
            {
                std::cerr << "Encountered an error of " << ex.what() << " upon opening " << fpath.string() << std::endl;;
                valid = false;
                break;
            }
        }
        if(valid){
            return Request(filePath, type);
        }
        else{
            return Request();
        }
    }
    else if(type == Client::History){
        return Request(filePath, type);
    }
    else{
        // invalid type
        return Request();
    }
}

std::string Client::Impl::sendRequest(const Client::Request& req){
    try{
        boost::asio::io_context io_context;

        // Get a list of endpoints corresponding to the server name.
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve("localhost", "http");

        // Try each endpoint until we successfully establish a connection.
        boost::asio::ip::tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        if(req.type() == Client::Person){
            std::stringstream msgBody;
            for(const auto& singlePath: req.filePath()){
                boost::filesystem::path fpath(singlePath);
                std::string shortFilename = fpath.filename().string();

                std::ifstream fileIn(singlePath, std::ios::binary | std::ios::ate);
                if(!fileIn){
                    std::cerr << "Unable to open file!" << std::endl;
                    exit(0);
                }
                std::streamsize size = fileIn.tellg();
                fileIn.seekg(0, std::ios::beg);

                char* data = new char[size];
                if (!fileIn.read(data, size))
                {
                    std::cerr << "Error reading contents!" << std::endl;
                    exit(0);
                }
                fileIn.close();

                std::string base64 = base64_encode(reinterpret_cast<const unsigned char*>(data), size);

                delete[] data;

                msgBody << "--77580b83-390b-4c34-8393-4eac360c7b42\r\n";
                msgBody << "Content-Disposition: form-data; name=\"datafile\"; filename=\"" << shortFilename <<"\"\r\n";
                msgBody << "Content-Type: image/jpeg\r\n\r\n";
                msgBody << base64 << "\r\n";
            }
            msgBody << "--77580b83-390b-4c34-8393-4eac360c7b42--\r\n";

            std::string stringRequestData = msgBody.str();
            
            // Form the request. We specify the "Connection: close" header so that the
            // server will close the socket after transmitting the response. This will
            // allow us to treat all data up until the EOF as the content.
            boost::asio::streambuf request;
            std::ostream request_stream(&request);
            request_stream << "POST " << "/predict" << " HTTP/1.0\r\n";
            // request_stream << "Host: " << "localhost" << "\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Content-Length: " << stringRequestData.length() << "\r\n";
            request_stream << "Content-Type: multipart/form-data; boundary=77580b83-390b-4c34-8393-4eac360c7b42\r\n";
            request_stream << "Connection: close\r\n\r\n";
            request_stream << stringRequestData;

            // std::stringstream testss;
            // testss << "POST " << "/predict" << " HTTP/1.0\r\n";
            // // testss << "Host: " << "localhost" << "\r\n";
            // testss << "Accept: */*\r\n";
            // testss << "Content-Length: " << stringRequestData.length() << "\r\n";
            // testss << "Content-Type: multipart/form-data; boundary=77580b83-390b-4c34-8393-4eac360c7b42\r\n";
            // testss << "Connection: close\r\n\r\n";
            // testss << stringRequestData;

            // std::cout<<"\r\n\r\nGoing to send "<<testss.str().length()<< std::endl;
            // std::cout<<testss.str()<<"end\r\n\r\n"<<std::endl;

            // Send the request.
            boost::asio::write(socket, request);

            // Read the response status line. The response streambuf will automatically
            // grow to accommodate the entire line. The growth may be limited by passing
            // a maximum size to the streambuf constructor.
            boost::asio::streambuf response;
            // boost::asio::read_until(socket, response, "jsonDataEnd.");
            boost::system::error_code ec;
            boost::asio::read(socket, response, boost::asio::transfer_all(), ec);
            if(ec == boost::asio::error::eof){
                ec.clear();
            }
            else
            {
                throw ec;
            }
            
            // Check that response is OK.
            std::istream response_stream(&response);
            return std::string(std::istreambuf_iterator<char>(response_stream), {});
        }
        else if(req.type() == Client::History){
            // Form the request. We specify the "Connection: close" header so that the
            // server will close the socket after transmitting the response. This will
            // allow us to treat all data up until the EOF as the content.
            boost::asio::streambuf request;
            std::ostream request_stream(&request);
            request_stream << "GET " << "/history" << " HTTP/1.0\r\n";
            // request_stream << "Host: " << "localhost" << "\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Connection: close\r\n\r\n";

            // std::stringstream testss;
            // testss << "GET " << "/history" << " HTTP/1.0\r\n";
            // // testss << "Host: " << "localhost" << "\r\n";
            // testss << "Accept: */*\r\n";
            // testss << "Connection: close\r\n\r\n";

            // std::cout<<"\r\n\r\nGoing to send "<<testss.str().length()<< std::endl;
            // std::cout<<testss.str()<<"end\r\n\r\n"<<std::endl;

            // Send the request.
            boost::asio::write(socket, request);

            // Read the response status line. The response streambuf will automatically
            // grow to accommodate the entire line. The growth may be limited by passing
            // a maximum size to the streambuf constructor.
            boost::asio::streambuf response;
            // boost::asio::read_until(socket, response, "jsonDataEnd.");
            boost::system::error_code ec;
            boost::asio::read(socket, response, boost::asio::transfer_all(), ec);
            if(ec == boost::asio::error::eof){
                ec.clear();
            }
            else
            {
                throw ec;
            }
            
            // Check that response is OK.
            std::istream response_stream(&response);
            return std::string(std::istreambuf_iterator<char>(response_stream), {});
        }

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return "";
    }

}

// std::string Client::Impl::base64Encode(const char* data, std::size_t size) const{
//     std::stringstream ss;
//     typedef 
//         boost::archive::iterators::base64_from_binary<
//            boost::archive::iterators::transform_width<const char *, 6, 8>
//         > 
//         base64_text;

//     std::copy(
//         base64_text(data),
//         base64_text(data + size),
//         boost::archive::iterators::ostream_iterator<char>(ss)
//     );

//     std::cout << ss.str();

//     return ss.str();
// }

Client::Client(){
    m_impl = std::unique_ptr<Impl>(new Impl);
}

Client::~Client(){

}

Client::Request Client::formRequest(const FilepathVec& filePath, const RequestType& type){
    return m_impl->formRequest(filePath, type);
}

std::string Client::sendRequest(const Request& req){
    return m_impl->sendRequest(req);
}

}