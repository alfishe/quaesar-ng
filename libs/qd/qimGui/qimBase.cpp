#include "qimBase.h"
#include "qimElement.h"

namespace qim
{

	bool Property::isSectEnterAllowed(EVisitStage curStage, qim::Context* ctx, ElementData* pData)
	{
	    if (isSectEnterAllowedImp(ctx, pData))
        {
            if (curStage.has(EVisitStage::VEventHandler) && m_classMeta->visitsAllowed.has(EVisitStage::VEventHandler))
            {
                if (pData->m_eventApplied.isDerivedFrom(m_classMeta->primeId))
                    return false; // already applied
                pData->m_eventApplied.addBaseClass(m_classMeta->primeId);
            }
            return true;
        }
	    return false;
	}


const qim::PropertyClassMeta& Property::gen_class_meta()
    {
        PropertyClassMeta& meta = qim::get_prop_class_meta_<Property>();
        if (meta.isDefined())
            return meta;
        meta.cid = qd::fnv1aHash(STRINGIFY(qim::Property));
        meta.sectType = ESectType::Proprty;
        meta.primeId = PrimeIdClassMgr::get().registerNewType();
        meta.visitsAllowed = EVisitStage::VProperty;
        return meta;
    }


const qim::PropertyClassMeta& Section::gen_class_meta()
    {
        PropertyClassMeta& meta = qim::get_prop_class_meta_<Section>();
        if (meta.isDefined())
            return meta;
        const PropertyClassMeta& parentMeta = Property::gen_class_meta();
        meta = parentMeta;
        meta.cid = qd::fnv1aHash(STRINGIFY(qim::Section));
        meta.sectType = ESectType::Section;
        meta.primeId = PrimeIdClassMgr::get().registerNewType();
        meta.visitsAllowed = EVisitStage::UNDEF;
        return meta;
    }


    }; // namespace qim
