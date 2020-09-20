#ifndef SISD_CLIENT_HPP
#define SISD_CLIENT_HPP

#include <vector>

#include <common/common.hpp>

namespace SISD{

class SISD_DECLSPEC Client final{
public:
    using FilepathVec = std::vector<std::string>;

    enum RequestType{
        Person = 0,
        Vehicle,
        Both,
        History,
        Invalid
    }; 

    class SISD_DECLSPEC Request final{
        friend class Client;
    public:
        Request(const Request&) = delete;

        Request(Request&& req);

        Request& operator=(const Request&) = delete;

        Request& operator=(Request&& req);

        operator bool() const;

        ~Request();

        const FilepathVec& filePath() const;

        RequestType type() const;

    private:
        Request() = default;

        Request(const FilepathVec& filePath, const RequestType& type);

        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

    Client();

    ~Client();

    Request formRequest(const FilepathVec& filePath, const RequestType& type);

    std::string sendRequest(const Request& req);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}


#endif //#ifndef SISD_CLIENT_HPP
