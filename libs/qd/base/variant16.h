#pragma once
#include "qd/base/baseTypes.h"
#include "qd/debug/assert.h"
#include "qd/stl/string.h"


namespace qd {


//////////////////////////////////////////////////////////////////////////
class Var16
{
    typedef Var16 TThis;

protected:
    static constexpr size_t g_sizeOf = 16;
    static constexpr size_t g_fullCapacity = g_sizeOf - 2;
    static constexpr size_t g_headDataCapacity = 8 - 2;
    static constexpr size_t g_bodyDataCapacity = 8;

    union {
        struct {
            uint8_t m_type;
            uint8_t m_dataSize; // strSize
            uint8_t _dataBufStart[g_headDataCapacity]; // str buf SSO
        };
        uint32_t _headData = 0;
    };
    union {
        int m_i32; // 32bit
        uint32_t m_u32;
        int m_bool;
        float m_f32;
        double m_f64;
        int64_t m_i64;
        uint64_t m_u64; // 8 byte
        void* m_pPointer; // 8 byte

        uint64_t _bodyData = 0;
    };


public:
    enum eType : uint8_t {
        DATA_NONE = 0,
        DATA_BOOL = 1,
        DATA_INT32 = 2,
        DATA_FLOAT = 3,
        DATA_UINT32 = 4,

        DATA_POINTER = 5,
        DATA_REF_PTR = 6,

        VAL_BOOL_TRUE = 7,
        VAL_BOOL_FALSE = 8,

        DATA_INT64 = 9,
        DATA_UINT64 = 10,
        DATA_DOUBLE = 11,

        DATA_EXTERNAL_PTR = 0x80,
        // HERE ARE STORING BIG TYPES IN EXTERNAL MEMORY STORAGE
        DATA_STRING,
        DATA_STRING_W,

        DATA_MAX_FORMATS,
        UNDEFINED_NULL = 0xFF,
    };


public:
    Var16() = default;
    ~Var16() = default;

    Var16::eType getType() const { return (Var16::eType)m_type; }

    inline bool isSet() const { return (m_type != DATA_NONE); }
    inline bool isValid() const { return c_def(this) && isSet(); }

    inline operator bool () const { return isValid(); }

public:
    Var16(int Value)
        : m_type(DATA_INT32)
        , m_i32(Value)
    {}

    Var16(bool bValue)
        : m_type(DATA_BOOL)
        , m_bool(bValue)
    {}


    Var16(float Value)
        : m_type(DATA_FLOAT)
        , m_f32(Value)
    {}

    Var16(uint32_t Value)
        : m_type(DATA_UINT32)
        , m_u32(Value)
    {}

    Var16(int64_t Value)
        : m_type(DATA_INT64)
        , m_i64(Value)
    {}

    Var16(uint64_t Value)
        : m_type(DATA_UINT64)
        , m_u64(Value)
    {}

    Var16(void* Value)
        : m_type(DATA_POINTER)
        , m_pPointer(Value)
    {}


    Var16(const char* pStr) { _setString(qd::string_view(pStr, strlen(pStr))); }

    template<int N>
    explicit Var16(const char (&pStr)[N])
    {
        _setString(qd::string_view(pStr, (uint32_t)(N - 1)));
    }

    Var16(const qd::string_view& Value) { _setString(Value); }

    Var16(const Var16& Clone) { this->operator= (Clone); }


    const Var16& operator= (const Var16& Clone);

    constexpr uint8_t capacity() const { return g_fullCapacity; }

    bool operator== (const Var16& r) const;

    inline bool operator!= (const Var16& r) const { return !(*this == r); }

public:
    inline void setI32(int Value)
    {
        assert(m_type == DATA_INT32 || m_type == DATA_NONE);
        m_i32 = Value;
        m_type = DATA_INT32;
    }

    inline void setBool(bool Value)
    {
        assert(m_type == DATA_BOOL || m_type == DATA_NONE);
        m_bool = (int)Value;
        m_type = DATA_BOOL;
    }

    inline void setF32(float Value)
    {
        assert(m_type == DATA_FLOAT || m_type == DATA_NONE);
        m_f32 = Value;
        m_type = DATA_FLOAT;
    }

    inline void setU32(uint32_t Value)
    {
        assert(m_type == DATA_UINT32 || m_type == DATA_NONE);
        m_u32 = Value;
        m_type = DATA_UINT32;
    }

    inline void setPtr(void* Value)
    {
        assert(m_type == DATA_POINTER || m_type == DATA_NONE);
        m_pPointer = Value;
        m_type = DATA_POINTER;
    }


    void setStr(const qd::string_view& Value);


    inline uint32_t getU32() const
    {
        assert(m_type == DATA_INT32 || m_type == DATA_UINT32);
        return m_u32;
    }
    inline bool getU32(uint32_t& bVal) const
    {
        if (m_type != DATA_INT32 && m_type != DATA_UINT32)
            return false;
        bVal = m_u32;
        return true;
    }

    inline int getI32() const
    {
        assert(m_type == DATA_INT32 || m_type == DATA_UINT32);
        return m_i32;
    }
    inline bool getI32(int& bVal) const
    {
        if (m_type != DATA_INT32 && m_type != DATA_UINT32)
            return false;
        bVal = m_i32;
        return true;
    }

    inline bool getBool() const
    {
        assert(m_type == DATA_BOOL);
        return m_bool != 0;
    }
    inline bool getBool(bool& bVal) const
    {
        if (m_type != DATA_BOOL)
            return false;
        bVal = (m_bool != 0);
        return true;
    }

    inline void* getPtr() const
    {
        assert(m_type == DATA_POINTER || m_type == DATA_REF_PTR ||
               (m_type > DATA_EXTERNAL_PTR && m_type < DATA_MAX_FORMATS));
        return m_pPointer;
    }


    const qd::string_view& getStr() const;

    inline bool getStr(qd::string_view& Out) const
    {
        if (m_type != DATA_STRING)
            return false;
        Out = getStr();
        return true;
    }

    inline float getF32() const
    {
        assert(m_type == DATA_FLOAT);
        return m_f32;
    }

    inline bool getF32(float& Out) const
    {
        if (m_type != DATA_FLOAT)
            return false;
        Out = m_f32;
        return true;
    }

    void set(void* Value) { setPtr(Value); }
    void set(bool Value) { setBool(Value); }
    void set(int Value) { setI32(Value); }
    void set(float Value) { setF32(Value); }
    void set(uint32_t Value) { setU32(Value); }
    void set(const char* Value) { setStr(qd::string_view(Value, strlen(Value))); }
    void set(const qd::string_view& Value) { setStr(Value); }


    struct RetZero {
        template<typename T>
        T operator() (const T& x) const
        {
            return std::move(x);
        }
    }; // struct


    bool toFloat(float& val) const;

    float toFloatDef(float def) const
    {
        float val;
        if (toFloat(val))
            return val;
        return def;
    }

    bool toInt32(int& val) const;

    int toI32(int def) const
    {
        int val;
        if (toInt32(val))
            return val;
        return def;
    }

protected:
    void _setString(const qd::string_view& Value);

    // FOR std::map Container
    bool operator< (const Var16& r) const;

    void reset();


public:
    static inline const Var16& Null();

}; // class Var16
//////////////////////////////////////////////////////////////////////////


static_assert(sizeof(Var16) == 16);


}; // namespace qd
