#ifndef SISD_CLIENT_HPP
#define SISD_CLIENT_HPP

#include <vector>

#include <common/common.hpp>

namespace SISD{

/**
* @brief The encapsulated client class sending request to server. This client always sends formated http
*       messages to localhost port 80
* 
* @param 
* @return 
* 
*/
class SISD_DECLSPEC Client final{
public:
    using FilepathVec = std::vector<std::string>;

    enum RequestType{
        Person = 0, // doing a person detection-classification pipeline
        Vehicle,    // doing a vehicle detection-classification pipeline. Not supported yet
        Both,       // doing both the person and vehicle pipelien. Not supported yet
        History,    // query the history inference results saved on server
        Invalid
    }; 

    /**
    * @brief the actual request that client sends. Users are expected to use Client::formRequest() to 
    *       construct a request 
    * 
    * @param 
    * @return 
    * 
    */
    class SISD_DECLSPEC Request final{
        friend class Client;
    public:
        Request(const Request&) = delete;

        Request(Request&& req);

        Request& operator=(const Request&) = delete;

        Request& operator=(Request&& req);

        /**
        * @brief Check if this request is valid
        * 
        * @param void
        * @return true if valid
        * 
        */
        operator bool() const;

        ~Request();

        /**
        * @brief returns a read reference to the file paths this request contains
        * 
        * @param void
        * @return const reference to a vector of strings
        * 
        */
        const FilepathVec& filePath() const;

        /**
        * @brief returns the type of this request
        * 
        * @param void
        * @return type this request belongs to
        * 
        */
        RequestType type() const;

    private:
        Request() = default;

        Request(const FilepathVec& filePath, const RequestType& type);

        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

    Client();

    ~Client();

    /**
    * @brief construct and returns a request. This function will also validate the parameters and in case of
    *       invalid settings, e.g. input paths do not exists, it will return a request that bool() to false
    * 
    * @param filePath vector of string containing image files, either relative or absolute
    * @param type request type, value could be any from struct Client::RequestType
    * @return the constructed request
    * 
    */
    Request formRequest(const FilepathVec& filePath, const RequestType& type);

    /**
    * @brief sends the request to server and return result in form of json string
    * 
    * @param req the request to be send
    * @return upon success it will return the json string, otherwise empty
    * 
    */
    std::string sendRequest(const Request& req);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}


#endif //#ifndef SISD_CLIENT_HPP
