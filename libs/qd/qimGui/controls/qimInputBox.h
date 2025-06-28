#pragma once
#include "qd/qimGui/qimGui.h"




namespace qim {
class UiInputBeh;


struct InputScalar : public qim::CtrlElement {
    QIM_ELEMENT_CLASS(qim::InputScalar, qim::CtrlElement, qim::UiInputBeh);
 protected:
    bool m_bTextChanged = false;
public:

    void setup(const char* text)
    { //
    }

    virtual bool isTextChanged() const
    {
        return m_bTextChanged;
    }

    //     virtual void onBegin(qim::Context* ctx) override;
    //     virtual void onEnd(qim::Context* ctx) override;

}; // struct
//////////////////////////////////////////////////////////////////////////



struct ValPtrStorage {
    static constexpr size_t MaxSize = 32;
    static constexpr size_t MaxAlign = alignof(std::max_align_t);
    alignas(MaxAlign) char m_storage[MaxSize];
    using Callback = bool (*)(void*, void*, void*);
    Callback m_pCastCb = nullptr;

    template<typename T>
    void bind(T&& val)
    {
        static_assert(sizeof(T) <= MaxSize, "Message too large");
        static_assert(alignof(T) <= MaxAlign, "Message alignment too big");

        m_pCastCb = [](void* pInstPtr, void* pOutVal, void* pInputVal) -> bool {
            auto* pCast = reinterpret_cast<T*>(pInstPtr);
            return pCast->cast(pOutVal, pInputVal);
        };
        new (m_storage) T(std::forward<T>(val));
    }

    void call(void* pOutVal, void* pInputVal)
    {
        if (m_pCastCb)
            m_pCastCb(m_storage, pOutVal, pInputVal);
    }
};


template<class T>
struct ToIntPtr
{
    T* m_pVal;

    ToIntPtr(T* p_val)
        : m_pVal(p_val)
    {}

    bool cast(void* pOutVal, void* pInputVal)
    {
        if (pOutVal)
        {
            *(int*)pOutVal = (int)(*m_pVal);
            return true;
        }
        if (pInputVal)
        {
            int val = *(int*)pInputVal;
            *m_pVal = (T)val;
            return true;
        }
        return false;
    }
};



struct InputInt : public qim::InputScalar {
    QIM_ELEMENT_CLASS(qim::InputInt, qim::InputScalar, qim::UiInputBeh);

    using StepInt = Props::StepInt;

    struct Imm {
        const char* m_label = nullptr;
        int* m_pVal = nullptr;
        ValPtrStorage m_valStorage;
    } im;

    void setup(const char* text, int* p_val)
    {
        setup(text, qim::ToIntPtr(p_val));
    }

    template<typename T>
    void setup(const char* text, qim::ToIntPtr<T>&& toInt)
    {
        im.m_label = text;
        im.m_valStorage.bind(std::forward<qim::ToIntPtr<T>>(toInt));
    }


    virtual void onDrawBeginImp(qim::Context* ctx) override {}

    virtual void onDrawEndImp(qim::Context* ctx) override;

}; // struct
//////////////////////////////////////////////////////////////////////////



class UiInputBeh : public qim::BehaviorElem
{
    TS_BEGIN_REFLECT_CLASS(qim::UiInputBeh, qim::BehaviorElem);
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemBehCb_<TRefClass>));
    TS_END();


public:

}; // class UiInputBeh
//////////////////////////////////////////////////////////////////////////



}; // namespace qim
