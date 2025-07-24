#pragma once
#include "qd/base/types.h"
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

    union
    {
        struct {
            uint8_t m_type;
            uint8_t m_dataSize; // strSize
            uint8_t _dataBufStart[g_headDataCapacity]; // str buf SSO
        };
        uint32_t _headData = 0;
    };
    union {
        int m_Int32; // 32bit
        uint32_t m_UInt32;
        int m_Bool;
        float m_Float;
        double m_Double;
        int64_t m_Int64;
        uint64_t m_UInt64; // 8 byte
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
        , m_Int32(Value)
    {}

    Var16(bool bValue)
        : m_type(DATA_BOOL)
        , m_Bool(bValue)
    {}


    Var16(float Value)
        : m_type(DATA_FLOAT)
        , m_Float(Value)
    {}

    Var16(uint32_t Value)
        : m_type(DATA_UINT32)
        , m_UInt32(Value)
    {}

    Var16(int64_t Value)
        : m_type(DATA_INT64)
        , m_Int64(Value)
    {}

    Var16(uint64_t Value)
        : m_type(DATA_UINT64)
        , m_UInt64(Value)
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
    inline void setInt(int Value)
    {
        assert(m_type == DATA_INT32 || m_type == DATA_NONE);
        m_Int32 = Value;
        m_type = DATA_INT32;
    }

    inline void setBool(bool Value)
    {
        assert(m_type == DATA_BOOL || m_type == DATA_NONE);
        m_Bool = (int)Value;
        m_type = DATA_BOOL;
    }

    inline void setFloat(float Value)
    {
        assert(m_type == DATA_FLOAT || m_type == DATA_NONE);
        m_Float = Value;
        m_type = DATA_FLOAT;
    }

    inline void setUInt32(uint32_t Value)
    {
        assert(m_type == DATA_UINT32 || m_type == DATA_NONE);
        m_UInt32 = Value;
        m_type = DATA_UINT32;
    }

    inline void setPtr(void* Value)
    {
        assert(m_type == DATA_POINTER || m_type == DATA_NONE);
        m_pPointer = Value;
        m_type = DATA_POINTER;
    }


    void setString(const qd::string_view& Value);


    inline uint32_t getUInt() const
    {
        assert(m_type == DATA_INT32 || m_type == DATA_UINT32);
        return m_UInt32;
    }
    inline bool getUInt(uint32_t& bVal) const
    {
        if (m_type != DATA_INT32 && m_type != DATA_UINT32)
            return false;
        bVal = m_UInt32;
        return true;
    }

    inline int getInt() const
    {
        assert(m_type == DATA_INT32 || m_type == DATA_UINT32);
        return m_Int32;
    }
    inline bool getInt(int& bVal) const
    {
        if (m_type != DATA_INT32 && m_type != DATA_UINT32)
            return false;
        bVal = m_Int32;
        return true;
    }

    inline bool getBool() const
    {
        assert(m_type == DATA_BOOL);
        return m_Bool != 0;
    }
    inline bool getBool(bool& bVal) const
    {
        if (m_type != DATA_BOOL)
            return false;
        bVal = (m_Bool != 0);
        return true;
    }

    inline void* getPtr() const
    {
        assert(m_type == DATA_POINTER || m_type == DATA_REF_PTR ||
               (m_type > DATA_EXTERNAL_PTR && m_type < DATA_MAX_FORMATS));
        return m_pPointer;
    }


    const qd::string_view& getString() const;

    inline bool getString(qd::string_view& Out) const
    {
        if (m_type != DATA_STRING)
            return false;
        Out = getString();
        return true;
    }

    inline float getFloat() const
    {
        assert(m_type == DATA_FLOAT);
        return m_Float;
    }

    inline bool getFloat(float& Out) const
    {
        if (m_type != DATA_FLOAT)
            return false;
        Out = m_Float;
        return true;
    }

    void set(void* Value) { setPtr(Value); }
    void set(bool Value) { setBool(Value); }
    void set(int Value) { setInt(Value); }
    void set(float Value) { setFloat(Value); }
    void set(uint32_t Value) { setUInt32(Value); }
    void set(const char* Value) { setString(qd::string_view(Value, strlen(Value))); }
    void set(const qd::string_view& Value) { setString(Value); }


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

    int toInt32Def(int def) const
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
