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


rem C:\cygwin64\home\John-Bradley\qemu-raspi\bin\debug\x86_64-w64-mingw64\i386-softmmu\qemu-system-i386.exe -L Bios -name linux-0.2 -drive file=linux-0.2.img >fred
%STR_PATH%/bin/debug/x86_64-w64-mingw64/x86_64-softmmu/qemu-system-x86_64.exe -L Bios -k fr -vga std -soundhw es1370 -boot menu=on -rtc base=localtime,clock=host -name linux-0.2 -drive file=linux-0.2.img,media=disk,cache=writeback -net nic,model=ne2k_pci -net user -no-acpi -no-hpet -no-reboot


cd %STR_PATH%
