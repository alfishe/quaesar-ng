#pragma once
#include "qimBase.h"
#include "qd/stl/vector_map.h"


namespace qim {

class Storage
{
    qd::vector_map<ElemId, ElemData*> m_dataMap;

public:
    ElemData* findDataById(ElemId id)
    {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end())
            return it->second;
        return nullptr;
    }

    void setData(ElemId slot_id, ElemData* p_data_inst) //
    {
        m_dataMap[slot_id] = p_data_inst;
    }

    void clear();

}; // class Storage
//////////////////////////////////////////////////////////////////////////


}; // namespace qim
