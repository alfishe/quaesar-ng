#pragma once
#include "qd/base/base.h"


namespace qd::net
{
class IObject;
class Message;
typedef uint16_t ObjectID;
typedef uint16_t MessageID;
constexpr static ObjectID INVALID_OBJECT_ID = ObjectID(-1);
constexpr static MessageID INVALID_MESSAGE_ID = MessageID(-1);


class IObject // base class for objects that handle messages
{
protected:
    ObjectID mpiObjectUID;

public:
    virtual ~IObject() {}
    IObject(ObjectID uid = INVALID_OBJECT_ID)
        : mpiObjectUID(uid)
    {}
    ObjectID getUID() const { return mpiObjectUID; }
    virtual Message* dispatchMpiMessage(MessageID mid) = 0; // construct message instance by message id
    virtual void applyMpiMessage(const Message* m) = 0; // execute message

}; // class IObject


}; // namespace qd::net
