#pragma once
#include "EASTL/intrusive_list.h"
#include "qd/base/base.h"
#include "qd/base/baseTypes.h"
#include "qd/debug/assert.h"
#include "qd/net/netObject.h"

FORWARD_DECLARATION_2(qd, BitStream);

#define BITS_TO_BYTES(x)              (((x) + 7) >> 3)
#define BITS_TO_BYTES_WORD_ALIGNED(x) ((((x) + 31) >> 5) << 2)
#define BYTES_TO_BITS(x)              ((x) << 3)
#define QDNET_REFLECTION_OP_ENCODE    0 // encode data
#define QDNET_REFLECTION_OP_DECODE    1 // decode data


namespace qd::net {
class ReflectionVarMeta;
class ReflectableObject;


#define REFL_VAR_DECL(num, decl_typ, dispatch_typ, var_name, var_flags, var_bits, coder_f_init, additional_decl, \
    init_code, templ)                                                                                            \
    class EventListenerFor_##num                                                                                 \
    {                                                                                                            \
        static_assert(::c_expr(num) > 0 && num <= SizeReflVarQuotaNumber);                                       \
                                                                                                                 \
    public:                                                                                                      \
        static inline void onVarCreated(qd::net::ReflectionVarMeta* meta)                                        \
        {                                                                                                        \
            qd::net::ReflectableObject* robj =                                                                   \
                (qd::net::ReflectableObject*)((char*)meta - offsetof(ThisClass, var_name));                      \
            on_reflection_var_created(meta, robj, StartReflVarQuotaNumber + num, #var_name, var_flags,           \
                var_bits ? var_bits : BYTES_TO_BITS(sizeof(dispatch_typ)));                                      \
            coder_f_init init_code                                                                               \
        }                                                                                                        \
        static inline void onStateChanged(qd::net::ReflectionVarMeta* meta, void* new_val)                       \
        {                                                                                                        \
            qd::net::ReflectableObject* robj =                                                                   \
                (qd::net::ReflectableObject*)((char*)meta - offsetof(ThisClass, var_name));                      \
            on_reflection_var_changed(meta, robj, new_val);                                                      \
        }                                                                                                        \
    };                                                                                                           \
    templ qd::net::ReflectionVarTypeDispatcher<decl_typ, EventListenerFor_##num>::VarType var_name;              \
    additional_decl


#define REFL_VAR(num, typ, var_name) REFL_VAR_DECL(num, typ, typ, var_name, 0, 0, QDNET_STD_ENCODER(typ), , , )


#define DECL_REFLECTION(class_name, base_class)                                      \
    typedef base_class BaseClass;                                                    \
    typedef class_name ThisClass;                                                    \
    static const int StartReflVarQuotaNumber = BaseClass::EndReflVarQuotaNumber + 1; \
    static const int EndReflVarQuotaNumber = StartReflVarQuotaNumber + ReflectableObject::SizeReflVarQuotaNumber;


#define QDNET_STD_ENCODER(t) meta->coder = qd::net::get_encoder_for((t*)0);
#define QDNET_RAW_ENCODER(t) meta->coder = qd::net::readwrite_var_raw<t>;
#define QDNET_ENCODER(t)     meta->coder = t;

//////////////////////////////////////////////////////////////////////////


enum ReflectionVarFlags {
    RVF_CHANGED = 1 << 0,
    RVF_EXCLUDED = 1 << 1,
    // marked this flags on deserialize, then called onBeforeVarsDeserialization(), if this method clears this flag -
    // that var will not
    // be deserialized
    RVF_NEED_DESERIALIZE = 1 << 7,
    // call ReflectableObject::onReflectionVarChanged() method when var changed
    RVF_CALL_HANDLER = 1 << 11,
    // like RVF_CALL_HANDLER, but works even with RVF_EXCLUDED (or ReflectableObject::EXCLUDED) flag
    RVF_CALL_HANDLER_FORCE = 1 << 5,
};


#define QDNET_ENCODER_SIGNATURE \
    int op, qd::net::ReflectionVarMeta *meta, const qd::net::ReflectableObject *ro, qd::BitStream *bs

typedef int (*reflection_var_encoder_cb)(QDNET_ENCODER_SIGNATURE);


extern void on_reflection_var_created(ReflectionVarMeta* meta, ReflectableObject* robj, uint8_t persistent_id,
    const char* var_name, uint16_t var_flags, uint16_t var_bits);
extern void on_reflection_var_changed(ReflectionVarMeta* meta, ReflectableObject* robj, void* new_val);


//------------------------------------------------------------------------
// vars of this type are linked in one list inside of ReflectableObject
class ReflectionVarMeta : public eastl::intrusive_list_node
{
public:
    uint8_t persistentId;
    uint16_t flags;
    uint16_t numBits;
    const char* name;
    reflection_var_encoder_cb coder;

    const char* getVarName() const { return name; }
    void* getValueRaw(size_t align) const
    {
        return (
            void*)(reinterpret_cast<const uint8_t*>(this) + ((sizeof(ReflectionVarMeta) + align - 1) / align) * align);
    }
    template<class T>
    T& getValue() const
    {
        return *(T*)getValueRaw(alignof(T));
    }
    void setChanged(bool f) { flags = f ? (flags | RVF_CHANGED) : (flags & ~RVF_CHANGED); }
}; // class ReflectionVarMeta



// class, you need inherit all your objects from
// all this objects are linked in all_reflectables list, as well may be in changed_reflectables list
class ReflectableObject : public qd::net::IObject
{
private:
    uint32_t reflectionFlags = EXCLUDED;

protected:
    static constexpr int EndReflVarQuotaNumber = 0;
    static constexpr int SizeReflVarQuotaNumber = 64;

public:
    constexpr static uint32_t DANET_WATERMARK = _MAKE4C('DNET');
    constexpr static uint32_t DANET_DEAD_WATERMARK = _MAKE4C('DEAD');

    enum Flags {
        /* free */
        EXCLUDED = 2, // this object will not synchronized (but will deserializes)
        /* free */
        REPLICATED = 8, // this is instance of ReplicatedObject
        FULL_SYNC = 16, // for this objects forced full sync of all wars
        DEBUG_REFLECTION = 32, // like define for all vars of this object RVF_DEBUG
    };
    eastl::intrusive_list<ReflectionVarMeta> varList;
    ReflectableObject* m_pParentRepObj = nullptr;
    uint32_t debugWatermark = DANET_WATERMARK;

public:

    ReflectableObject(qd::net::ObjectID uid = INVALID_OBJECT_ID, ReflectableObject* pParentRepObj = nullptr)
        : qd::net::IObject(uid)
        , m_pParentRepObj(pParentRepObj)
    {}

    virtual ~ReflectableObject()
    {
        checkWatermark();
        disableReflection(true);
        debugWatermark = DANET_DEAD_WATERMARK;
    }

public:
    void checkWatermark() const
    {
        if (debugWatermark != DANET_WATERMARK)
            ASSERT_F(0, "Reflection: invalid object 0x%p", this);
    }

    void setRelfectionFlag(Flags flag) { reflectionFlags |= flag; }
    bool isRelfectionFlagSet(Flags flag) const { return (bool)(reflectionFlags & flag); }
    uint32_t getRelfectionFlags() const { return reflectionFlags; }

    virtual Message* dispatchMpiMessage(MessageID mid) override { return nullptr; }
    virtual void applyMpiMessage(const Message* m) override {}

    int calcNumVars() const
    {
        checkWatermark();
        return (int)varList.size();
    }

    void disableReflection(bool full)
    {
        checkWatermark();

        resetChangeFlag();
        reflectionFlags |= EXCLUDED;
    }

    const ReflectionVarMeta* findVarByName(const char* name) const
    {
        checkWatermark();

        for (const ReflectionVarMeta& m : varList)
            if (strcmp(m.getVarName(), name) == 0)
                return &m;
        return nullptr;
    }

    const ReflectionVarMeta* findVarByPersistentId(int id) const
    {
        for (const ReflectionVarMeta& m : varList)
            if (id == m.persistentId)
                return &m;
        return nullptr;
    }

    void resetChangeFlag()
    {
        checkWatermark();

        reflectionFlags &= ~FULL_SYNC;
        for (ReflectionVarMeta& m : varList)
            m.flags &= ~RVF_CHANGED;

        // DANET_DLIST_REMOVE(changed_reflectables, this, prevChanged, nextChanged);
    }

    bool isReflObjChanged() const
    {
        return true; // prevChanged || this == changed_reflectables.head; // TODO: circular linked list
    }

    void forceFullSync()
    {
        checkWatermark();

        if (reflectionFlags & EXCLUDED)
            return;
        for (ReflectionVarMeta& m : varList)
            if ((m.flags & RVF_EXCLUDED) == 0)
                m.flags |= RVF_CHANGED;
        reflectionFlags |= FULL_SYNC;
        markAsChanged();
    }

    void markAsChanged() { assert(0); }

    // called after every var change (if RVF_CALL_HANDLER specified for it)
    virtual void onReflectionVarChanged(ReflectionVarMeta*, void* /*newVal*/) {}


}; // class ReflectableObject
//////////////////////////////////////////////////////////////////////////



// this class define base operations with reflection var
template<typename T, class EventListener>
class ReflectionVarBase : public ReflectionVarMeta
{
public:
    typedef T ElemType;

    ReflectionVarBase()
        : ReflectionVarMeta()
    {
        EventListener::onVarCreated(this);
    }

    explicit ReflectionVarBase(const T& other)
        : ReflectionVarMeta()
    {
        EventListener::onVarCreated(this);
        ReflectionVarMeta::getValue<T>() = other;
    }

    ReflectionVarBase(const ReflectionVarBase& other)
        : ReflectionVarBase(other.get())
    {} // trivial copy ctor in order to make class non trivially copyable

    void markAsChanged(void* new_val = NULL)
    {
        EventListener::onStateChanged(this, new_val ? new_val : getValueRaw(alignof(T)));
    }

    T& getForModify()
    {
        markAsChanged();
        return ReflectionVarMeta::getValue<T>();
    }

    const T& get() const { return ReflectionVarMeta::getValue<T>(); }
    operator const T& () const { return get(); }

    T operator->() const
    {
        // static_assert(IsPtr<T>::Value == true);
        return ReflectionVarMeta::getValue<T>();
    }

    const T& Set(const T& new_val)
    {
        T& val = ReflectionVarMeta::getValue<T>();
        if (val != new_val)
        {
            markAsChanged((void*)&new_val);
            val = new_val;
        }
        return val;
    }

    template<typename T1>
    const T& operator= (const T1& val)
    {
        return Set((const T)val);
    }

    template<typename T1>
    const T& operator= (const ReflectionVarBase<T1, EventListener>& val)
    {
        return Set((const T)val.ReflectionVarMeta::template getValue<T1>());
    }

    template<typename T1>
    bool operator== (const T1& a) const
    {
        return ReflectionVarMeta::getValue<T>() == a;
    }

    template<typename T1>
    bool operator!= (const T1& a) const
    {
        return ReflectionVarMeta::getValue<T>() != a;
    }

    template<typename T1>
    bool operator== (const ReflectionVarBase<T1, EventListener>& a) const
    {
        return ReflectionVarMeta::getValue<T>() == a.template getValue<T1>();
    }

    template<typename T1>
    bool operator!= (const ReflectionVarBase<T1, EventListener>& a) const
    {
        return ReflectionVarMeta::getValue<T>() != a.template getValue<T1>();
    }
}; // class ReflectionVarBase
//////////////////////////////////////////////////////////////////////////


// this class actually contains data for all types except vectors
template<typename T, class EventListener>
class ReflectionVarScalar : public ReflectionVarBase<T, EventListener>
{
public:
    T value;
    typedef ReflectionVarBase<T, EventListener> BaseClass;

    ReflectionVarScalar()
        : ReflectionVarBase<T, EventListener>()
    {}

    explicit ReflectionVarScalar(const T& other)
        : ReflectionVarBase<T, EventListener>(other)
    {}

    template<typename T1>
    const T& operator= (const T1& val)
    {
        return BaseClass::Set((const T)val);
    }

    template<typename T1>
    const T& operator= (const ReflectionVarBase<T1, EventListener>& val)
    {
        return BaseClass::Set((const T)val.ReflectionVarMeta::template getValue<T1>());
    }
}; // class ReflectionVarScalar
//////////////////////////////////////////////////////////////////////////



template<typename T, class EventListener>
struct ReflectionVarTypeDispatcher // default type
{
    typedef ReflectionVarScalar<T, EventListener> VarType;
};


template<typename T>
int readwrite_var_raw(QDNET_ENCODER_SIGNATURE)
{
    if (op == QDNET_REFLECTION_OP_ENCODE)
    {
        //bs->Write(meta->getValue<T>());
    }
    else if (op == QDNET_REFLECTION_OP_DECODE)
    {
        if (meta->flags & RVF_NEED_DESERIALIZE)
        {
            if (!(meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE)))
                return -1; // bs->Read(meta->getValue<T>());
            else
            {
/*
                T new_val;
                if (!bs->Read(new_val))
                    return 0;
                T& val = meta->getValue<T>();
                if (val != new_val)
                {
                    const_cast<ReflectableObject*>(ro)->onReflectionVarChanged(meta, &new_val);
                    val = new_val;
                }
*/
            }
        }
        else
        {
            //T tempVar;
            //bs->Read(tempVar); // skip sizeof(T) bytes
        }
    }
    return -1;
}


template<typename T>  // default coder (instantiated when no overloaded function found)
static inline reflection_var_encoder_cb get_encoder_for(T*)
{
    return &readwrite_var_raw<T>;
}

}; // namespace qd::net
