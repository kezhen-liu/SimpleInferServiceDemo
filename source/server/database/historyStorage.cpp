#include <mutex>
#include <sw/redis++/redis++.h>

#include <server/database/historyStorage.hpp>

namespace SISD{

class HistoryStorage::Impl{
public:

    Impl();

    ~Impl();

    JobHandle generateHandle();

    bool save(JobHandle handle, const std::string& json);

    bool getAll(std::unordered_map<std::string, std::string>& res);

private:
    uint32_t m_ctr;
    std::unique_ptr<sw::redis::Redis> m_ctxP;
    std::mutex m_ctrMutex;
};

HistoryStorage::Impl::Impl():m_ctr(0u){
    m_ctxP = std::unique_ptr<sw::redis::Redis>(new sw::redis::Redis("tcp://127.0.0.1:6379"));
}

HistoryStorage::Impl::~Impl(){

}

HistoryStorage::JobHandle HistoryStorage::Impl::generateHandle(){
    std::lock_guard<std::mutex> lg(m_ctrMutex);
    JobHandle ret = m_ctr++;
    return ret;
}

bool HistoryStorage::Impl::save(JobHandle handle, const std::string& json){
    m_ctxP->hset("history", std::make_pair(std::to_string(handle), json));
    return true;
}

bool HistoryStorage::Impl::getAll(std::unordered_map<std::string, std::string>& res){
    res.clear();
    m_ctxP->hgetall("history", std::inserter(res, res.begin()));
    return true;
}

HistoryStorage::~HistoryStorage(){

}

HistoryStorage& HistoryStorage::getInstance(){
    static HistoryStorage inst;
    return inst;
}

HistoryStorage::JobHandle HistoryStorage::generateHandle(){
    return m_impl->generateHandle();
}

bool HistoryStorage::save(JobHandle handle, const std::string& json){
    return m_impl->save(handle, json);
}

bool HistoryStorage::getAll(std::unordered_map<std::string, std::string>& res){
    return m_impl->getAll(res);
}

HistoryStorage::HistoryStorage(){
    m_impl = std::unique_ptr<Impl>(new Impl);
}

}