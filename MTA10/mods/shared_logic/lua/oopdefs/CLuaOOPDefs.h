/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*               (Shared logic for modifications)
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        mods/shared_logic/lua/CLuaFunctionDefs.h
*  PURPOSE:     Lua function definitions class header
*  DEVELOPERS:  Stanislav Bobrov <lil_toady@hotmail.com>
*
*****************************************************************************/

class CLuaFunctionDefinitions;

#ifndef __CLUAOOPDEFINITIONS_H
#define __CLUAOOPDEFINITIONS_H

#include "../LuaCommon.h"
#include "../CLuaMain.h"

class CRegisteredCommands;

#define LUA_DECLARE(x) static int x ( lua_State * luaVM );

class CLuaOOPDefs : public CLuaDefs
{
public:
    // Element
    LUA_DECLARE ( GetElementMatrix );
    LUA_DECLARE ( GetElementPosition );
    LUA_DECLARE ( GetElementRotation );
    LUA_DECLARE ( SetElementRotation );
    LUA_DECLARE ( GetElementVelocity );

    // Camera
    LUA_DECLARE ( GetCameraMatrix );
    LUA_DECLARE ( GetCameraPosition );
    LUA_DECLARE ( SetCameraPosition );
    LUA_DECLARE ( GetCameraRotation );
    LUA_DECLARE ( SetCameraRotation );

    // Vehicle
    LUA_DECLARE ( GetVehicleGravity );
    LUA_DECLARE ( GetVehicleTurnVelocity );

    // Marker
    LUA_DECLARE ( GetMarkerTarget );

    // Drawing
    LUA_DECLARE ( DxGetFontHeight );
    LUA_DECLARE ( DxGetTextWidth );

    // Radar Area
    LUA_DECLARE ( GetRadarAreaSize );
};

#endif
