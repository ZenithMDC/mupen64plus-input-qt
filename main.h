#ifndef __MAIN_H__
#define __MAIN_H__

#include <QString>
#include <SDL2/SDL.h>

typedef struct
{
    CONTROL *control;               // pointer to CONTROL struct in Core library
    SDL_GameController *gamepad;
    SDL_Joystick *joystick;
    QString profile;
    int deadzone;
    int range;
    float sensitivity;
    SDL_Haptic* haptic;
} SController;

extern "C"
{
    EXPORT void CALL PluginConfig();
}

extern void (*debug_callback)(void *, int, const char *);
extern void *debug_call_context;

// Some stuff from n-rage plugin
#define RD_GETSTATUS        0x00        // get status
#define RD_READKEYS         0x01        // read button values
#define RD_READPAK          0x02        // read from controllerpack
#define RD_WRITEPAK         0x03        // write to controllerpack
#define RD_RESETCONTROLLER  0xff        // reset controller
#define RD_READEEPROM       0x04        // read eeprom
#define RD_WRITEEPROM       0x05        // write eeprom

#define PAK_IO_RUMBLE       0xC000      // the address where rumble-commands are sent to

#endif
