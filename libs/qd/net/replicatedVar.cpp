#include "replicatedVar.h"


namespace qd::net {


void on_reflection_var_created(ReflectionVarMeta* meta, ReflectableObject* robj, uint8_t persistent_id,
    const char* var_name, uint16_t var_flags, uint16_t var_bits)
{
    robj->checkWatermark();
#if _DEBUG
    if (!(var_flags & RVF_EXCLUDED))
    {
        const ReflectionVarMeta* sameNameMeta = robj->findVarByName(var_name);
        ASSERT_F(!sameNameMeta, "duplicate reflection var %s", var_name);
        const ReflectionVarMeta* sameIdMeta = robj->findVarByPersistentId(persistent_id);
        ASSERT_F(!sameIdMeta, "duplicate reflection var id %d", persistent_id);
    }
#endif

    robj->varList.push_back(*meta);

    meta->persistentId = persistent_id;
    meta->name = var_name;
    meta->flags = var_flags;
    meta->numBits = var_bits;
}


void on_reflection_var_changed(ReflectionVarMeta* meta, ReflectableObject* robj, void* new_val)
{
    robj->checkWatermark();
    if ((meta->flags & RVF_EXCLUDED) || (robj->isRelfectionFlagSet(ReflectableObject::EXCLUDED)))
    {
        if (meta->flags & RVF_CALL_HANDLER_FORCE)
            robj->onReflectionVarChanged(meta, new_val);
        return;
    }
    if (!(meta->flags & RVF_CHANGED))
    {
        meta->flags |= RVF_CHANGED;
        //DANET_DLIST_PUSH(danet::changed_reflectables, robj, prevChanged, nextChanged);
    }
    if (meta->flags & (qd::net::RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE))
        robj->onReflectionVarChanged(meta, new_val);
}


}; // namespace qd
