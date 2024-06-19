#ifndef _CALL_CALL_STATE_H
#define _CALL_CALL_STATE_H
namespace call
{
enum CallState
{
    CALL_STATE_FAILED      = -1,
    CALL_STATE_NA          = 0,
    CALL_STATE_TRYING      = 1,
    CALL_STATE_RINGING     = 2,
    CALL_STATE_ACCEPT      = 3,
    CALL_STATE_TRANSFERRED = 4,
    CALL_STATE_HOLD        = 5,
    CALL_STATE_UNHOLD      = 6,
    CALL_STATE_HANGUP      = 7,
    CALL_STATE_CLOSED      = 8
};
}
#endif//_CALL_CALL_STATE_H