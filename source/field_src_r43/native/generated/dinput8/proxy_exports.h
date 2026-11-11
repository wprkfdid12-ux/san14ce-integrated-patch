#pragma once
#define FL_PROXY_SYSTEM_NAME L"dinput8.dll"
#define FL_PROXY_SLOT 1
#define FL_PROXY_EXPORT_COUNT 6
static const char *fl_proxy_export_keys[]={"DirectInput8Create","DllCanUnloadNow","DllGetClassObject","DllRegisterServer","DllUnregisterServer","GetdfDIJoystick"};
