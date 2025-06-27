#include "qd/base/classPrimeId.h"
#include "qd/base/primesArray.h"
#include "qd/debug/assert.h"


namespace qd
{


bool ClassPrimeId::isDerivedFrom(const ClassPrimeId& rh) const {
    assert(rh.m_primeId != 0);
    return (m_primeId % rh.m_primeId) == 0;
}



const qd::ClassPrimeId& ClassPrimeIdMgr::registerNewType()
{
    uint32_t nType = (uint32_t)m_storage.size();
    ClassPrimeId& cpid = m_storage.emplace_back();
    cpid.m_primeId = index_to_prime(nType);

    return cpid;
}


constexpr uint32_t index_to_prime(uint32_t ind)
{
    return g_PrimesArray[ind];
}


}; // namespace qd
