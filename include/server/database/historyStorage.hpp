#ifndef SISD_HISTORY_STORAGE_HPP
#define SISD_HISTORY_STORAGE_HPP

#include <vector>
#include <unordered_map>

#include <common/common.hpp>

namespace SISD{

class SISD_DECLSPEC HistoryStorage final{
public:
    using JobHandle = uint32_t;

    ~HistoryStorage();

    HistoryStorage(const HistoryStorage&) = delete;

    HistoryStorage(HistoryStorage&&) = delete;

    HistoryStorage& operator=(const HistoryStorage&) = delete;

    HistoryStorage& operator=(HistoryStorage&&) = delete;

    static HistoryStorage& getInstance();

    JobHandle generateHandle();

    bool save(JobHandle handle, const std::string& json);

    bool getAll(std::unordered_map<std::string, std::string>& res);

private:
    HistoryStorage();

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}

#endif //#ifndef SISD_HISTORY_STORAGE_HPP
