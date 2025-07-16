#include "stringId.h"
#include "qd/stl/string.h"
#include <EASTL/fixed_string.h>
#include <EASTL/hash_map.h>
#include <qd/debug/assert.h>
#include <qd/mem/fnvHash.h>
#include <qd/thread/thread.h>


//-------------------------------------------------------------------------

namespace qd {
class StringIDHashMap : public eastl::hash_map<uint64_t, qd::string>
{
public:
    eastl::hash_node<value_type, false> const* const* GetBuckets() const { return mpBucketArray; }
};

//-------------------------------------------------------------------------


static qd::Mutex* g_pStringCacheMutex = nullptr;
static StringIDHashMap* g_pStringCache = nullptr;

//-------------------------------------------------------------------------

void StringID::Initialize()
{
    g_pStringCacheMutex = new qd::Mutex();
    g_pStringCache = new StringIDHashMap();
}

void StringID::Shutdown()
{
    delete g_pStringCache;
    delete g_pStringCacheMutex;
}

//-------------------------------------------------------------------------

StringID::StringID(char const* pStr)
{
    // If this is nullptr then you are likely trying to statically allocate a stringID, this is not allowed and you need
    // to use the "StaticStringID" type instead!
    //assert(g_pStringCacheMutex != nullptr);

    if (pStr != nullptr && strlen(pStr) > 0)
    {
        m_ID = fnv1aHash(pStr);

        // Cache the string
        qd::MutexLock lock(*g_pStringCacheMutex);
        auto iter = g_pStringCache->find(m_ID);
        if (iter == g_pStringCache->end())
        {
            (*g_pStringCache)[m_ID] = string(pStr);
        }
        else
        {
            assert(iter->second == pStr);
        }
    }
}

StringID::StringID(string const& str)
    : StringID(str.c_str())
{}

StringID::StringID(InlineString const& str)
    : StringID(str.c_str())
{}

char const* StringID::c_str() const
{
    if (m_ID == 0)
    {
        return nullptr;
    }

    {
        // Get cached string
        MutexLock lock(*g_pStringCacheMutex);
        auto iter = g_pStringCache->find(m_ID);
        if (iter != g_pStringCache->end())
        {
            return iter->second.c_str();
        }
    }

    // ID likely directly created via uint64_t
    return nullptr;
}

//-------------------------------------------------------------------------

StaticStringID::StaticStringID(char const* pStr)
{
    if (pStr)
    {
        size_t const length = strlen(pStr);
        assert(length < 64);
        memcpy(m_buffer, pStr, length);
    }
}

}; // namespace qd
