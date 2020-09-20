#ifndef SISD_PERSON_PIPELINE_HPP
#define SISD_PERSON_PIPELINE_HPP

#include <common/common.hpp>

namespace SISD{

class SISD_DECLSPEC PersonPipeline{
public:
    PersonPipeline();

    virtual ~PersonPipeline();

    bool init();

    std::string run(const char* input, std::size_t size, const std::string& imageName);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}

#endif //#ifndef SISD_PERSON_PIPELINE_HPP
