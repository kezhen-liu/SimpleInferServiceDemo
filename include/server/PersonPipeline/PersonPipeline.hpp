#ifndef SISD_PERSON_PIPELINE_HPP
#define SISD_PERSON_PIPELINE_HPP

#include <common/common.hpp>

namespace SISD{

/**
* @brief This class wraps the entire person decoding-detection-classification pipeline.
*       For now the decoding is soft-decoding using openCV. Inference workload is carried
*       on CPU using Intel Openvino toolkit
* 
* @param 
* @return 
* 
*/
class SISD_DECLSPEC PersonPipeline{
public:
    /**
    * @brief construct a pipeline
    * 
    * @param void
    * @return 
    * 
    */
    PersonPipeline();

    virtual ~PersonPipeline();

    /**
    * @brief initialize the pipeline before running
    * 
    * @param void
    * @return true if success
    * 
    */
    bool init();

    /**
    * @brief run the pipeline with supplied image
    * 
    * @param input binnary input of compressed media type e.g. jpeg
    * @param size length of the binary input
    * @param imageName the name of image which will be used in result json and database
    * @return result json string
    * 
    */
    std::string run(const char* input, std::size_t size, const std::string& imageName);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}

#endif //#ifndef SISD_PERSON_PIPELINE_HPP
