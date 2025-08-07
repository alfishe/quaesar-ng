#pragma once
#include "debuggerOps.h"


namespace amD {

class IDbgConnection
{
public:
    virtual void pushOperation(amD::operation::OperationArgs*) = 0;
    virtual amD::operation::OperationArgs* frontOperation() = 0;
    virtual void popFrontOperation() {}

}; // IDbgConnection
//////////////////////////////////////////////////////////////////////////



}; // namespace amD
