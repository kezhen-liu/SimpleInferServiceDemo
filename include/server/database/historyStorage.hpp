#ifndef SISD_HISTORY_STORAGE_HPP
#define SISD_HISTORY_STORAGE_HPP

#include <vector>
#include <unordered_map>

#include <common/common.hpp>

namespace SISD{

/**
* @brief the wrapper of database storing history records. Current implementation is Redis, which is a fast
*       in-memory database
* 
* @param 
* @return 
* 
*/
class SISD_DECLSPEC HistoryStorage final{
public:
    using JobHandle = uint32_t;

    ~HistoryStorage();

    HistoryStorage(const HistoryStorage&) = delete;

    HistoryStorage(HistoryStorage&&) = delete;

    HistoryStorage& operator=(const HistoryStorage&) = delete;

    HistoryStorage& operator=(HistoryStorage&&) = delete;

    /**
    * @brief get a reference to the global singleton
    * 
    * @param void
    * @return reference to HistoryStorage
    * 
    */
    static HistoryStorage& getInstance();

    /**
    * @brief generate a unique job handle that will be used for later save operation
    * 
    * @param void
    * @return a uint32_t handle
    * 
    */
    JobHandle generateHandle();

    /**
    * @brief save a json record to database
    * 
    * @param handle the unique handle generated using generateHandle()
    * @param json the string to be saved
    * @return true if success
    * 
    */
    bool save(JobHandle handle, const std::string& json);

    /**
    * @brief retrieve all records available in database
    * 
    * @param res the destination buffer
    * @return true if success
    * 
    */
    bool getAll(std::unordered_map<std::string, std::string>& res);

private:
    HistoryStorage();

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}

#endif //#ifndef SISD_HISTORY_STORAGE_HPP
