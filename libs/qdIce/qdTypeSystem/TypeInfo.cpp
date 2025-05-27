#include "TypeInfo.h"

//-------------------------------------------------------------------------

namespace qd {
bool TypeInfo::IsDerivedFrom(TypeId const potentialParentTypeID) const {
    if (potentialParentTypeID == m_ID) {
        return true;
    }

    if (m_pParentTypeInfo != nullptr) {
        if (m_pParentTypeInfo->m_ID == potentialParentTypeID) {
            return true;
        }

        // Check inheritance hierarchy
        if (m_pParentTypeInfo->IsDerivedFrom(potentialParentTypeID)) {
            return true;
        }
    }

    return false;
}

};  //namespace qd
