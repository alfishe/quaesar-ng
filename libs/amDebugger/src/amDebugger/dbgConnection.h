#pragma once
#include "debuggerOps.h"
#include "qd/stl/ref_ptr.h"


namespace amD {
class DbgSharedConnectionImp;


class IDbgConnection : public qd::RefCounted
{
public:
    qd::string m_name;

public:
    virtual void pushOperation(amD::operation::OperationArgs*) = 0;
    virtual amD::operation::OperationArgs* getFrontOperation() = 0;
    virtual void popFrontOperation() {}

}; // IDbgConnection
//////////////////////////////////////////////////////////////////////////


ref_ptr<IDbgConnection> create_dummy_connection();
ref_ptr<IDbgConnection> create_shared_connection(const char* name);


}; // namespace amD
