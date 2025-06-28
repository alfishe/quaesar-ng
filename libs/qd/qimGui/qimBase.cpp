#include "qimBase.h"
#include "qimElement.h"
#include "qimGui.h"


namespace qim
{


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
