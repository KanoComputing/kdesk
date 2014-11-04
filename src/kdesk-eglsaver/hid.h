//
//   hid.h - definitions for Human Input Device
//

#include <stdbool.h>

// Defaut generous buffer size to read from devices
#define BUFF_SIZE         128
#define GRACE_START_TIME  300   // milliseconds to wait for type-ahead input events

// Devices to which we are listening for user events
#define chkbd0    "/dev/input/event0"
#define chkbd1    "/dev/input/event1"
#define chkbd2    "/dev/input/event2"
#define chmouse0  "/dev/input/mouse0"
#define chmouse1  "/dev/input/mouse1"
#define chmice    "/dev/input/mice"

// Structure to hold the devices to listen to
typedef struct _HID_STRUCT
{
    // File-like handles to user input devices
    int fdkbd0, fdkbd1, fdkbd2, fdmouse0, fdmouse1, fdmice;

} HID_STRUCT;

typedef HID_STRUCT *HID_HANDLE;

// APIs to init, terminate, and get events from HID devices


#ifdef __cplusplus
extern "C"
#endif
HID_HANDLE hid_init(int flags);

#ifdef __cplusplus
extern "C"
#endif
bool hid_is_user_idle (HID_HANDLE hhid, int timeout);

#ifdef __cplusplus
extern "C"
#endif
void hid_terminate(HID_HANDLE hid);
