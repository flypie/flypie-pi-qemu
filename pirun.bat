rem @ECHO OFF


SETLOCAL ENABLEEXTENSIONS
SET me=%~n0

set STR_PATH=%~dp0
cd C:/qemu


REM SDL_VIDEODRIVER=directx is faster than windib. But keyboard cannot work well.
SET SDL_VIDEODRIVER=windib

REM QEMU_AUDIO_DRV=dsound or fmod or sdl or none can be used. See qemu -audio-help.
SET QEMU_AUDIO_DRV=dsound

REM SDL_AUDIODRIVER=waveout or dsound can be used. Only if QEMU_AUDIO_DRV=sdl.
SET SDL_AUDIODRIVER=dsound

REM QEMU_AUDIO_LOG_TO_MONITOR=1 displays log messages in QEMU monitor.
SET QEMU_AUDIO_LOG_TO_MONITOR=1


%STR_PATH%bin\debug\x86_64-w64-mingw64\arm-softmmu\qemu-system-arm.exe -machine raspi2 -smp 1 -bios C:\cygwin64\home\John-Bradley\circle\sample\03-screentext\kernel7.img -usbdevice keyboard
cd %STR_PATH%
