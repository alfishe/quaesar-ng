#pragma once


namespace qim
{

template<typename T>
class qptr
{
private:
	T* m_ptr;

public:
	qptr() = default;
	qptr(T* ptr)
	    : m_ptr(ptr)
	{}

	T* get() const { return m_ptr.get(); }

	T& operator* () const
	{
	    assert(m_ptr);
	    return *m_ptr;
	}

	T* operator->() const
	{
	    assert(m_ptr);
	    return m_ptr;
	}

	explicit operator bool () const { return static_cast<bool>(m_ptr); }

	qptr(const qptr&) = delete; // Disable copy constructor
	qptr& operator= (const qptr&) = delete;

	qptr(qptr&& other) noexcept
	    : m_ptr(other.m_ptr)
	{
	    other.m_ptr = nullptr; // Transfer ownership
	}

	qptr& operator= (qptr&& other) noexcept
	{
	    if (this != &other)
	    {
	        if (m_ptr)
	            qim::endCtrl(m_ptr);
	        m_ptr = other.m_ptr;
	        other.m_ptr = nullptr;
	    }
	    return *this;
	}

	qptr(T* ptr, bool takeOwnership)
	    : m_ptr(ptr)
	{
	    if (takeOwnership && m_ptr)
	    {
	        qim::beginCtrl(m_ptr); // Assuming beginCtrl takes ownership of the pointer
	    }
	}

	T* release()
	{
	    T* temp = m_ptr;
	    m_ptr = nullptr; // Release ownership
	    return temp;
	}

	~qptr();
};


}; // namespace qim
