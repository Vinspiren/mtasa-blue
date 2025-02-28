#pragma message("Compiling precompiled header.\n")

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#define MTA_CLIENT
#define SHARED_UTIL_WITH_FAST_HASH_MAP
#include "SharedUtil.h"

#include <string.h>
#include <stdio.h>
#include <mmsystem.h>
#include <winsock.h>

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstring>

#include <base64.h>
#include <zlib.h>

// SDK includes
#include <core/CLocalizationInterface.h>
#include <core/CCoreInterface.h>
#include <core/CExceptionInformation.h>
#include <xml/CXML.h>
#include <xml/CXMLNode.h>
#include <xml/CXMLFile.h>
#include <xml/CXMLAttributes.h>
#include <xml/CXMLAttribute.h>
#include <net/CNet.h>
#include <net/packetenums.h>
#include <game/CGame.h>
#include <CVector.h>
#include <CVector4D.h>
#include <CMatrix4.h>
#include <CQuat.h>
#include <CSphere.h>
#include <CBox.h>
#include <ijsify.h>
#include <Common.h>
#include "net/Packets.h"
#include "Enums.h"
#include "net/SyncStructures.h"
#include "CIdArray.h"
#include "pcrecpp.h"

// Shared logic includes
#include <Utils.h>
#include <CClientCommon.h>
#include <CClientManager.h>
#include <CClient3DMarker.h>
#include <CClientCheckpoint.h>
#include <CClientColShape.h>
#include <CClientColCircle.h>
#include <CClientColCuboid.h>
#include <CClientColSphere.h>
#include <CClientColRectangle.h>
#include <CClientColPolygon.h>
#include <CClientColTube.h>
#include <CClientCorona.h>
#include <CClientDFF.h>
#include <CClientDummy.h>
#include <CClientEntity.h>
#include <CClientSpatialDatabase.h>
#include <CClientExplosionManager.h>
#include <CClientPed.h>
#include <CClientPlayerClothes.h>
#include <CClientPlayerVoice.h>
#include <CClientPointLights.h>
#include <CClientProjectileManager.h>
#include <CClientStreamSector.h>
#include <CClientStreamSectorRow.h>
#include <CClientTask.h>
#include <CClientTXD.h>
#include <CClientWater.h>
#include <CClientWeapon.h>
#include <CClientRenderElement.h>
#include <CClientDxFont.h>
#include <CClientGuiFont.h>
#include <CClientMaterial.h>
#include <CClientTexture.h>
#include <CClientShader.h>
#include <CClientWebBrowser.h>
#include <CClientSearchLight.h>
#include <CClientEffect.h>
#include <CCustomData.h>
#include <CElementArray.h>
#include <CLogger.h>
#include <CMapEventManager.h>
#include <CModelNames.h>
#include <CScriptFile.h>
#include <CWeaponNames.h>
#include <CVehicleNames.h>
#include <lua/CLuaCFunctions.h>
#include <lua/CLuaArguments.h>
#include <lua/CLuaMain.h>
#include "CEasingCurve.h"
#include <lua/CLuaFunctionParseHelpers.h>
#include <CScriptArgReader.h>
#include <luadefs/CLuaDefs.h>
#include <luadefs/CLuaBrowserDefs.h>
#include <luadefs/CLuaClassDefs.h>
#include <luadefs/CLuaPointLightDefs.h>
#include <luadefs/CLuaPedDefs.h>
#include <luadefs/CLuaVector2Defs.h>
#include <luadefs/CLuaVector3Defs.h>
#include <luadefs/CLuaVector4Defs.h>
#include <luadefs/CLuaMatrixDefs.h>
#include <luadefs/CLuaSearchLightDefs.h>
#include <luadefs/CLuaTaskDefs.h>
#include <luadefs/CLuaFxDefs.h>
#include <luadefs/CLuaFileDefs.h>
#include <lua/oopdefs/CLuaOOPDefs.h>
#include <CRemoteCalls.h>

// Shared includes
#include "TInterpolation.h"
#include "CPositionRotationAnimation.h"
#include "CLatentTransferManager.h"
#include "CDebugHookManager.h"
#include "CLuaShared.h"

// Deathmatch includes
#include "ClientCommands.h"
#include "CClient.h"
#include "CEvents.h"
#include "HeapTrace.h"
#include "logic/CClientGame.h"
#include "logic/CGameEntityXRefManager.h"
#include "logic/CClientModelCacheManager.h"
#include "logic/CClientPerfStatManager.h"
#include "logic/CDeathmatchVehicle.h"
#include "logic/CResource.h"
#include "logic/CStaticFunctionDefinitions.h"
#include "logic/CResourceFileDownloadManager.h"
#include "../../version.h"


