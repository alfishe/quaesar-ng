#include <qdIce/qdBase/classIdPrime.h>
#include <qdIce/qdBase/primesArray.h>
#include <qdIce/qdDebug/assert.h>


namespace qd
{

	
bool ClassIdPrime::isSubClassOf(const ClassIdPrime& rh) const {
    assert(rh.classId != 0);
    return (classId % rh.classId) == 0;
}


constexpr uint32_t ClassIdPrime::indToPrime(uint32_t ind) {
    return g_PrimesArray[ind];
}


}; // namespace qd
