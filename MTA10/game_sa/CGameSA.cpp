/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        game_sa/CGameSA.cpp
*  PURPOSE:     Base game logic handling
*  DEVELOPERS:  Ed Lyons <eai@opencoding.net>
*               Christian Myhre Lundheim <>
*               Jax <>
*               Cecill Etheredge <ijsf@gmx.net>
*               Stanislav Bobrov <lil_toady@hotmail.com>
*               Alberto Alonso <rydencillo@gmail.com>
*               Sebas Lamers <sebasdevelopment@gmx.com>
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#include "StdInc.h"
#define ALLOC_STATS_MODULE_NAME "game_sa"
#include "SharedUtil.hpp"
#include "SharedUtil.MemAccess.hpp"

unsigned long* CGameSA::VAR_SystemTime;
unsigned long* CGameSA::VAR_IsAtMenu;
unsigned long* CGameSA::VAR_IsGameLoaded;
bool* CGameSA::VAR_GamePaused;
bool* CGameSA::VAR_IsForegroundWindow;
unsigned long* CGameSA::VAR_SystemState;
void* CGameSA::VAR_StartGame;
bool* CGameSA::VAR_IsNastyGame;
float* CGameSA::VAR_TimeScale;
float* CGameSA::VAR_FPS;
float* CGameSA::VAR_OldTimeStep;
float* CGameSA::VAR_TimeStep;
unsigned long* CGameSA::VAR_Framelimiter;

/**
 * \todo allow the addon to change the size of the pools (see 0x4C0270 - CPools::Initialise) (in start game?)
 */
CGameSA::CGameSA()
{
    pGame = this;
    m_bAsyncScriptEnabled = false;
    m_bAsyncScriptForced = false;
    m_bASyncLoadingSuspended = false;
    m_iCheckStatus = 0;

    SetInitialVirtualProtect();

    // Initialize the offsets
    eGameVersion version = FindGameVersion ();
    switch ( version )
    {
        case VERSION_EU_10: COffsets::Initialize10EU (); break;
        case VERSION_US_10: COffsets::Initialize10US (); break;
        case VERSION_11:    COffsets::Initialize11 (); break;
        case VERSION_20:    COffsets::Initialize20 (); break;
    }

    // Set the model ids for all the CModelInfoSA instances
    for ( int i = 0; i < MODELINFO_MAX; i++ )
    {
        ModelInfo [i].SetModelID ( i );
    }

    DEBUG_TRACE("CGameSA::CGameSA()");
    this->m_pAudioEngine            = new CAudioEngineSA((CAudioEngineSAInterface*)CLASS_CAudioEngine);
    this->m_pAudioContainer         = new CAudioContainerSA();
    this->m_pWorld                  = new CWorldSA();
    this->m_pPools                  = new CPoolsSA();
    this->m_pClock                  = new CClockSA();
    this->m_pRadar                  = new CRadarSA();
    this->m_pCamera                 = new CCameraSA((CCameraSAInterface *)CLASS_CCamera);
    this->m_pCoronas                = new CCoronasSA();
    this->m_pCheckpoints            = new CCheckpointsSA();
    this->m_pPickups                = new CPickupsSA();
    this->m_pExplosionManager       = new CExplosionManagerSA();
    this->m_pHud                    = new CHudSA();
    this->m_pFireManager            = new CFireManagerSA();
    this->m_p3DMarkers              = new C3DMarkersSA();
    this->m_pPad                    = new CPadSA((CPadSAInterface *)CLASS_CPad);
    this->m_pTheCarGenerators       = new CTheCarGeneratorsSA();
    this->m_pCAERadioTrackManager   = new CAERadioTrackManagerSA();
    this->m_pWeather                = new CWeatherSA();
    this->m_pMenuManager            = new CMenuManagerSA();
    this->m_pText                   = new CTextSA();
    this->m_pStats                  = new CStatsSA();
    this->m_pFont                   = new CFontSA();
    this->m_pPathFind               = new CPathFindSA();
    this->m_pPopulation             = new CPopulationSA();
    this->m_pTaskManagementSystem   = new CTaskManagementSystemSA();
    this->m_pSettings               = new CSettingsSA();
    this->m_pCarEnterExit           = new CCarEnterExitSA();
    this->m_pControllerConfigManager = new CControllerConfigManagerSA();
    this->m_pProjectileInfo         = new CProjectileInfoSA();
    this->m_pRenderWare             = new CRenderWareSA( version );
    this->m_pHandlingManager        = new CHandlingManagerSA ();
    this->m_pEventList              = new CEventListSA();
    this->m_pGarages                = new CGaragesSA ( (CGaragesSAInterface *)CLASS_CGarages);
    this->m_pTasks                  = new CTasksSA ( (CTaskManagementSystemSA*)m_pTaskManagementSystem );
    this->m_pAnimManager            = new CAnimManagerSA;
    this->m_pStreaming              = new CStreamingSA;
    this->m_pVisibilityPlugins      = new CVisibilityPluginsSA;
    this->m_pKeyGen                 = new CKeyGenSA;
    this->m_pRopes                  = new CRopesSA;
    this->m_pFx                     = new CFxSA ( (CFxSAInterface *)CLASS_CFx );
    this->m_pFxManager              = new CFxManagerSA ( (CFxManagerSAInterface *)CLASS_CFxManager );
    this->m_pWaterManager           = new CWaterManagerSA ();
    this->m_pWeaponStatsManager     = new CWeaponStatManagerSA ();
    this->m_pPointLights            = new CPointLightsSA ();

    // Normal weapon types (WEAPONSKILL_STD)
    for ( int i = 0; i < NUM_WeaponInfosStdSkill; i++)
    {
        eWeaponType weaponType = (eWeaponType)(WEAPONTYPE_PISTOL + i);
        WeaponInfos[i] = new CWeaponInfoSA( (CWeaponInfoSAInterface *)(ARRAY_WeaponInfo + i*CLASSSIZE_WeaponInfo), weaponType );
        m_pWeaponStatsManager->CreateWeaponStat ( WeaponInfos[i], (eWeaponType)(weaponType - WEAPONTYPE_PISTOL), WEAPONSKILL_STD );
    }

    // Extra weapon types for skills (WEAPONSKILL_POOR,WEAPONSKILL_PRO,WEAPONSKILL_SPECIAL)
    int index;
    eWeaponSkill weaponSkill = eWeaponSkill::WEAPONSKILL_POOR;
    for ( int skill = 0; skill < 3 ; skill++ )
    {
        //STD is created first, then it creates "extra weapon types" (poor, pro, special?) but in the enum 1 = STD which meant the STD weapon skill contained pro info
        if ( skill >= 1 )
        {
            if ( skill == 1 )
            {
                weaponSkill = eWeaponSkill::WEAPONSKILL_PRO;
            }
            if ( skill == 2 )
            {
                weaponSkill = eWeaponSkill::WEAPONSKILL_SPECIAL;
            }
        }
        for ( int i = 0; i < NUM_WeaponInfosOtherSkill; i++ )
        {
            eWeaponType weaponType = (eWeaponType)(WEAPONTYPE_PISTOL + i);
            index = NUM_WeaponInfosStdSkill + skill*NUM_WeaponInfosOtherSkill + i;
            WeaponInfos[index] = new CWeaponInfoSA( (CWeaponInfoSAInterface *)(ARRAY_WeaponInfo + index*CLASSSIZE_WeaponInfo), weaponType );
            m_pWeaponStatsManager->CreateWeaponStat ( WeaponInfos[index], weaponType, weaponSkill );
        }
    }

    m_pPlayerInfo = new CPlayerInfoSA ( (CPlayerInfoSAInterface *)CLASS_CPlayerInfo );

    // Init cheat name => address map
    m_Cheats [ CHEAT_HOVERINGCARS     ] = new SCheatSA((BYTE *)VAR_HoveringCarsEnabled);
    m_Cheats [ CHEAT_FLYINGCARS       ] = new SCheatSA((BYTE *)VAR_FlyingCarsEnabled);
    m_Cheats [ CHEAT_EXTRABUNNYHOP    ] = new SCheatSA((BYTE *)VAR_ExtraBunnyhopEnabled);
    m_Cheats [ CHEAT_EXTRAJUMP        ] = new SCheatSA((BYTE *)VAR_ExtraJumpEnabled);

    // New cheats for Anticheat
    m_Cheats [ CHEAT_TANKMODE         ] = new SCheatSA((BYTE *)VAR_TankModeEnabled, false);
    m_Cheats [ CHEAT_NORELOAD         ] = new SCheatSA((BYTE *)VAR_NoReloadEnabled, false);
    m_Cheats [ CHEAT_PERFECTHANDLING  ] = new SCheatSA((BYTE *)VAR_PerfectHandling, false);
    m_Cheats [ CHEAT_ALLCARSHAVENITRO ] = new SCheatSA((BYTE *)VAR_AllCarsHaveNitro, false);
    m_Cheats [ CHEAT_BOATSCANFLY      ] = new SCheatSA((BYTE *)VAR_BoatsCanFly, false);
    m_Cheats [ CHEAT_INFINITEOXYGEN   ] = new SCheatSA((BYTE *)VAR_InfiniteOxygen, false);
    m_Cheats [ CHEAT_WALKUNDERWATER   ] = new SCheatSA((BYTE *)VAR_WalkUnderwater, false);
    m_Cheats [ CHEAT_FASTERCLOCK      ] = new SCheatSA((BYTE *)VAR_FasterClock, false);
    m_Cheats [ CHEAT_FASTERGAMEPLAY   ] = new SCheatSA((BYTE *)VAR_FasterGameplay, false);
    m_Cheats [ CHEAT_SLOWERGAMEPLAY   ] = new SCheatSA((BYTE *)VAR_SlowerGameplay, false);
    m_Cheats [ CHEAT_ALWAYSMIDNIGHT   ] = new SCheatSA((BYTE *)VAR_AlwaysMidnight, false);
    m_Cheats [ CHEAT_FULLWEAPONAIMING ] = new SCheatSA((BYTE *)VAR_FullWeaponAiming, false);
    m_Cheats [ CHEAT_INFINITEHEALTH   ] = new SCheatSA((BYTE *)VAR_InfiniteHealth, false);
    m_Cheats [ CHEAT_NEVERWANTED      ] = new SCheatSA((BYTE *)VAR_NeverWanted, false);
    m_Cheats [ CHEAT_HEALTARMORMONEY  ] = new SCheatSA((BYTE *)VAR_HealthArmorMoney, false);

    // Change pool sizes here
    m_pPools->SetPoolCapacity ( TASK_POOL, 5000 );                  // Default is 500
    m_pPools->SetPoolCapacity ( OBJECT_POOL, 700 );                 // Default is 350
    m_pPools->SetPoolCapacity ( EVENT_POOL, 5000 );                 // Default is 200
    m_pPools->SetPoolCapacity ( COL_MODEL_POOL, 12000 );            // Default is 10150
    m_pPools->SetPoolCapacity ( ENV_MAP_MATERIAL_POOL, 16000 );     // Default is 4096
    m_pPools->SetPoolCapacity ( ENV_MAP_ATOMIC_POOL, 4000 );        // Default is 1024
    m_pPools->SetPoolCapacity ( SPEC_MAP_MATERIAL_POOL, 16000 );    // Default is 4096

    // Increase streaming object instances list size
    MemPut < WORD > ( 0x05B8E55, 30000 );         // Default is 12000
    MemPut < WORD > ( 0x05B8EB0, 30000 );         // Default is 12000

    CModelInfoSA::StaticSetHooks ();
    CPlayerPedSA::StaticSetHooks ();
    CRenderWareSA::StaticSetHooks ();
    CRenderWareSA::StaticSetClothesReplacingHooks ();
    CTasksSA::StaticSetHooks ();
    CPedSA::StaticSetHooks ();
    CSettingsSA::StaticSetHooks ();
    CFxSystemSA::StaticSetHooks ();
}

CGameSA::~CGameSA ( void )
{
    delete reinterpret_cast < CPlayerInfoSA* > ( m_pPlayerInfo );

    
    for ( int i = 0; i < NUM_WeaponInfosTotal; i++ )
    {
        delete reinterpret_cast < CWeaponInfoSA* > ( WeaponInfos [i] );
    }

    delete reinterpret_cast < CFxSA * > ( m_pFx );
    delete reinterpret_cast < CRopesSA * > ( m_pRopes );
    delete reinterpret_cast < CKeyGenSA * > ( m_pKeyGen );
    delete reinterpret_cast < CVisibilityPluginsSA * > ( m_pVisibilityPlugins );
    delete reinterpret_cast < CStreamingSA * > ( m_pStreaming );
    delete reinterpret_cast < CAnimManagerSA* > ( m_pAnimManager );
    delete reinterpret_cast < CTasksSA* > ( m_pTasks );
    delete reinterpret_cast < CTaskManagementSystemSA* > ( m_pTaskManagementSystem );
    delete reinterpret_cast < CHandlingManagerSA* > ( m_pHandlingManager );
    delete reinterpret_cast < CPopulationSA* > ( m_pPopulation );
    delete reinterpret_cast < CPathFindSA* > ( m_pPathFind );
    delete reinterpret_cast < CFontSA* > ( m_pFont );
    delete reinterpret_cast < CStatsSA* > ( m_pStats );
    delete reinterpret_cast < CTextSA* > ( m_pText );
    delete reinterpret_cast < CMenuManagerSA* > ( m_pMenuManager );
    delete reinterpret_cast < CWeatherSA* > ( m_pWeather );
    delete reinterpret_cast < CAERadioTrackManagerSA* > ( m_pCAERadioTrackManager );
    delete reinterpret_cast < CTheCarGeneratorsSA* > ( m_pTheCarGenerators );
    delete reinterpret_cast < CPadSA* > ( m_pPad );
    delete reinterpret_cast < C3DMarkersSA* > ( m_p3DMarkers );
    delete reinterpret_cast < CFireManagerSA* > ( m_pFireManager );
    delete reinterpret_cast < CHudSA* > ( m_pHud );
    delete reinterpret_cast < CExplosionManagerSA* > ( m_pExplosionManager );
    delete reinterpret_cast < CPickupsSA* > ( m_pPickups );
    delete reinterpret_cast < CCheckpointsSA* > ( m_pCheckpoints );
    delete reinterpret_cast < CCoronasSA* > ( m_pCoronas );
    delete reinterpret_cast < CCameraSA* > ( m_pCamera );
    delete reinterpret_cast < CRadarSA* > ( m_pRadar );
    delete reinterpret_cast < CClockSA* > ( m_pClock );
    delete reinterpret_cast < CPoolsSA* > ( m_pPools );
    delete reinterpret_cast < CWorldSA* > ( m_pWorld );
    delete reinterpret_cast < CAudioEngineSA* > ( m_pAudioEngine );
    delete reinterpret_cast < CAudioContainerSA* > ( m_pAudioContainer );
    delete reinterpret_cast < CPointLightsSA * > ( m_pPointLights );
}

CWeaponInfo * CGameSA::GetWeaponInfo(eWeaponType weapon, eWeaponSkill skill)
{ 
    DEBUG_TRACE("CWeaponInfo * CGameSA::GetWeaponInfo(eWeaponType weapon)");
    
    if ( (skill == WEAPONSKILL_STD && weapon >= WEAPONTYPE_UNARMED && weapon < WEAPONTYPE_LAST_WEAPONTYPE) ||
         (skill != WEAPONSKILL_STD && weapon >= WEAPONTYPE_PISTOL && weapon <= WEAPONTYPE_TEC9) )
    {
        int offset = 0;
        switch ( skill )
        {
            case WEAPONSKILL_STD:
                offset = 0;
                break;
            case WEAPONSKILL_POOR:
                offset = NUM_WeaponInfosStdSkill - WEAPONTYPE_PISTOL;
                break;
            case WEAPONSKILL_PRO:
                offset = NUM_WeaponInfosStdSkill + NUM_WeaponInfosOtherSkill - WEAPONTYPE_PISTOL;
                break;
            case WEAPONSKILL_SPECIAL:
                offset = NUM_WeaponInfosStdSkill + 2*NUM_WeaponInfosOtherSkill - WEAPONTYPE_PISTOL;
                break;
            default:
                break;
        }
        return WeaponInfos[offset + weapon]; 
    }
    else 
        return NULL; 
}

VOID CGameSA::Pause ( bool bPaused )
{
    *VAR_GamePaused = bPaused;
}

bool CGameSA::IsPaused ( )
{
    return *VAR_GamePaused;
}

bool CGameSA::IsInForeground ()
{
    return *VAR_IsForegroundWindow;
}

CModelInfo  * CGameSA::GetModelInfo(DWORD dwModelID )
{ 
    DEBUG_TRACE("CModelInfo * CGameSA::GetModelInfo(DWORD dwModelID )");
    if (dwModelID < MODELINFO_MAX) 
    {
        if ( ModelInfo [dwModelID ].IsValid () )
        {
            return &ModelInfo[dwModelID];
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL; 
    }
}

/**
 * Starts the game
 * \todo make addresses into constants
 */
VOID CGameSA::StartGame()
{
    DEBUG_TRACE("VOID CGameSA::StartGame()");
//  InitScriptInterface();
    //*(BYTE *)VAR_StartGame = 1;
    this->SetSystemState(GS_INIT_PLAYING_GAME);
    MemPutFast < BYTE > ( 0xB7CB49, 0 );
    MemPutFast < BYTE > ( 0xBA67A4, 0 );
}

/**
 * Sets the part of the game loading process the game is in.
 * @param dwState DWORD containing a valid state 0 - 9
 */
VOID CGameSA::SetSystemState( eSystemState State )
{
    DEBUG_TRACE("VOID CGameSA::SetSystemState( eSystemState State )");
    *VAR_SystemState = (DWORD)State;
}

eSystemState CGameSA::GetSystemState( )
{
    DEBUG_TRACE("eSystemState CGameSA::GetSystemState( )");
    return (eSystemState)*VAR_SystemState;
}

/**
 * This adds the local player to the ped pool, nothing else
 * @return BOOL TRUE if success, FALSE otherwise
 */
BOOL CGameSA::InitLocalPlayer(  )
{
    DEBUG_TRACE("BOOL CGameSA::InitLocalPlayer(  )");

    // Added by ChrML - Looks like it isn't safe to call this more than once but mod code might do
    static bool bAlreadyInited = false;
    if ( bAlreadyInited )
    {
        return TRUE;
    }
    bAlreadyInited = true;

    CPoolsSA * pools = (CPoolsSA *)this->GetPools ();
    if ( pools )
    {
        //* HACKED IN HERE FOR NOW *//
        CPedSAInterface* pInterface = pools->GetPedInterface ( (DWORD)1 );

        if ( pInterface )
        {
            pools->AddPed ( (DWORD*)pInterface );
            return TRUE;
        }

        return FALSE;
    }
    return FALSE;
}

float CGameSA::GetGravity ( void )
{
    return * ( float* ) ( 0x863984 );
}

void CGameSA::SetGravity ( float fGravity )
{
    MemPut < float > ( 0x863984, fGravity );
}

float CGameSA::GetGameSpeed ( void )
{
    return * ( float* ) ( 0xB7CB64 );
}

void CGameSA::SetGameSpeed ( float fSpeed )
{
    MemPutFast < float > ( 0xB7CB64, fSpeed );
}

// this prevents some crashes (respawning mainly)
VOID CGameSA::DisableRenderer( bool bDisabled )
{
    // ENABLED:
    // 0053DF40   D915 2C13C800    FST DWORD PTR DS:[C8132C]
    // DISABLED:
    // 0053DF40   C3               RETN

    if ( bDisabled )
    {
        MemPut < BYTE > ( 0x53DF40, 0xC3 );
    }
    else
    {
        MemPut < BYTE > ( 0x53DF40, 0xD9 );
    }
}

VOID CGameSA::SetRenderHook ( InRenderer* pInRenderer )
{
    if ( pInRenderer )
        HookInstall ( (DWORD)FUNC_CDebug_DebugDisplayTextBuffer, (DWORD)pInRenderer, 6 );
    else
    {
        MemPut < BYTE > ( FUNC_CDebug_DebugDisplayTextBuffer, 0xC3 );
    }
}


VOID CGameSA::TakeScreenshot ( char * szFileName )
{
    DWORD dwFunc = FUNC_JPegCompressScreenToFile;
    _asm
    {
        mov     eax, CLASS_RwCamera
        mov     eax, [eax]
        push    szFileName
        push    eax
        call    dwFunc
        add     esp,8
    }
}

DWORD * CGameSA::GetMemoryValue ( DWORD dwOffset )
{
    if ( dwOffset <= MAX_MEMORY_OFFSET_1_0 )
        return (DWORD *)dwOffset;
    else
        return NULL;
}

void CGameSA::Reset ( void )
{
    // Things to do if the game was loaded
    if ( GetSystemState () == GS_PLAYING_GAME )
    {
        // Extinguish all fires
        m_pFireManager->ExtinguishAllFires();

        // Restore camera stuff
        m_pCamera->Restore ();
        m_pCamera->SetFadeColor ( 0, 0, 0 );
        m_pCamera->Fade ( 0, FADE_OUT );

        Pause ( false );        // We don't have to pause as the fadeout will stop the sound. Pausing it will make the fadein next start ugly
        m_pHud->Disable ( false );

        DisableRenderer ( false );

        // Restore the HUD
        m_pHud->Disable ( false );
        m_pHud->SetComponentVisible ( HUD_ALL, true );
    }
}

void CGameSA::Terminate ( void )
{
    // Initiate the destruction
    delete this;

    // Dump any memory leaks if DETECT_LEAK is defined
    #ifdef DETECT_LEAKS    
        DumpUnfreed();
    #endif
}

void CGameSA::Initialize ( void )
{
    // Initialize garages
    m_pGarages->Initialize();
    SetupSpecialCharacters ();
    m_pRenderWare->Initialize();

    // *Sebas* Hide the GTA:SA Main menu.
    MemPutFast < BYTE > ( CLASS_CMenuManager+0x5C, 0 );
}

eGameVersion CGameSA::GetGameVersion ( void )
{
    return m_eGameVersion;
}

eGameVersion CGameSA::FindGameVersion ( void )
{
    unsigned char ucA = *reinterpret_cast < unsigned char* > ( 0x748ADD );
    unsigned char ucB = *reinterpret_cast < unsigned char* > ( 0x748ADE );
    if ( ucA == 0xFF && ucB == 0x53 )
    {
        m_eGameVersion = VERSION_US_10;
    }
    else if ( ucA == 0x0F && ucB == 0x84 )
    {
        m_eGameVersion = VERSION_EU_10;
    }
    else if ( ucA == 0xFE && ucB == 0x10 )
    {
        m_eGameVersion = VERSION_11;
    }
    else
    {
        m_eGameVersion = VERSION_UNKNOWN;
    }

    return m_eGameVersion;
}

float CGameSA::GetFPS ( void )
{
    return *VAR_FPS;
}

float CGameSA::GetTimeStep ( void )
{
    return *VAR_TimeStep;
}

float CGameSA::GetOldTimeStep ( void )
{
    return *VAR_OldTimeStep;
}

float CGameSA::GetTimeScale ( void )
{
    return *VAR_TimeScale;
}

void CGameSA::SetTimeScale ( float fTimeScale )
{
    *VAR_TimeScale = fTimeScale;
}


unsigned char CGameSA::GetBlurLevel ( void )
{
    return * ( unsigned char * ) 0x8D5104;
}


void CGameSA::SetBlurLevel ( unsigned char ucLevel )
{
    MemPutFast < unsigned char > ( 0x8D5104, ucLevel );
}

unsigned long CGameSA::GetMinuteDuration ( void )
{
    return * ( unsigned long * ) 0xB7015C;
}


void CGameSA::SetMinuteDuration ( unsigned long ulTime )
{
    MemPutFast < unsigned long > ( 0xB7015C, ulTime );
}

bool CGameSA::IsCheatEnabled ( const char* szCheatName )
{
    std::map < std::string, SCheatSA* >::iterator it = m_Cheats.find ( szCheatName );
    if ( it == m_Cheats.end () )
        return false;
    return *(it->second->m_byAddress) != 0;
}

bool CGameSA::SetCheatEnabled ( const char* szCheatName, bool bEnable )
{
    std::map < std::string, SCheatSA* >::iterator it = m_Cheats.find ( szCheatName );
    if ( it == m_Cheats.end () )
        return false;
    if ( !it->second->m_bCanBeSet )
        return false;
    MemPutFast < BYTE > ( it->second->m_byAddress, bEnable );
    it->second->m_bEnabled = bEnable;
    return true;
}

void CGameSA::ResetCheats ()
{
    std::map < std::string, SCheatSA* >::iterator it;
    for ( it = m_Cheats.begin (); it != m_Cheats.end (); it++ ) {
        if ( it->second->m_byAddress > (BYTE*)0x8A4000 )
            MemPutFast < BYTE > ( it->second->m_byAddress, 0 );
        else
            MemPut < BYTE > ( it->second->m_byAddress, 0 );
        it->second->m_bEnabled = false;
    }
}

bool CGameSA::GetJetpackWeaponEnabled ( eWeaponType weaponType )
{
    if ( weaponType >= WEAPONTYPE_BRASSKNUCKLE && weaponType < WEAPONTYPE_LAST_WEAPONTYPE )
    {
        return m_JetpackWeapons[weaponType];
    }
    return false;
}

void CGameSA::SetJetpackWeaponEnabled ( eWeaponType weaponType, bool bEnabled )
{
    if ( weaponType >= WEAPONTYPE_BRASSKNUCKLE && weaponType < WEAPONTYPE_LAST_WEAPONTYPE )
    {
        m_JetpackWeapons[weaponType] = bEnabled;
    }
}

bool CGameSA::PerformChecks ( void )
{
    std::map < std::string, SCheatSA* >::iterator it;
    for ( it = m_Cheats.begin (); it != m_Cheats.end (); it++ ) {
        if (*(it->second->m_byAddress) != BYTE(it->second->m_bEnabled))
            return false;
    }
    return true;
}
bool CGameSA::VerifySADataFileNames ()
{
    return !strcmp ( *(char **)0x5B65AE, "DATA\\CARMODS.DAT" ) &&
           !strcmp ( *(char **)0x5BD839, "DATA" ) &&
           !strcmp ( *(char **)0x5BD84C, "HANDLING.CFG" ) &&
           !strcmp ( *(char **)0x5BEEE8, "DATA\\melee.dat" ) &&
           !strcmp ( *(char **)0x4D563E, "ANIM\\PED.IFP" ) &&
           !strcmp ( *(char **)0x5B925B, "DATA\\OBJECT.DAT" ) &&
           !strcmp ( *(char **)0x55D0FC, "data\\surface.dat" ) &&
           !strcmp ( *(char **)0x55F2BB, "data\\surfaud.dat" ) &&
           !strcmp ( *(char **)0x55EB9E, "data\\surfinfo.dat" ) &&
           !strcmp ( *(char **)0x6EAEF8, "DATA\\water.dat" ) &&
           !strcmp ( *(char **)0x6EAEC3, "DATA\\water1.dat" ) &&
           !strcmp ( *(char **)0x5BE686, "DATA\\WEAPON.DAT" );
}

void CGameSA::SetAsyncLoadingFromScript ( bool bScriptEnabled, bool bScriptForced )
{
    m_bAsyncScriptEnabled = bScriptEnabled;
    m_bAsyncScriptForced = bScriptForced;
}

void CGameSA::SuspendASyncLoading ( bool bSuspend, uint uiAutoUnsuspendDelay )
{
    m_bASyncLoadingSuspended = bSuspend;
    // Setup auto unsuspend time if required
    if ( uiAutoUnsuspendDelay && bSuspend )
        m_llASyncLoadingAutoUnsuspendTime = CTickCount::Now() + CTickCount( (long long)uiAutoUnsuspendDelay );
    else
        m_llASyncLoadingAutoUnsuspendTime = CTickCount();
}

bool CGameSA::IsASyncLoadingEnabled ( bool bIgnoreSuspend )
{
    // Process auto unsuspend time if set
    if ( m_llASyncLoadingAutoUnsuspendTime.ToLongLong() != 0 )
    {
        if ( CTickCount::Now() > m_llASyncLoadingAutoUnsuspendTime )
        {
            m_llASyncLoadingAutoUnsuspendTime = CTickCount();
            m_bASyncLoadingSuspended = false;
        }
    }

    if ( m_bASyncLoadingSuspended && !bIgnoreSuspend )
        return false;

    if ( m_bAsyncScriptForced )
        return m_bAsyncScriptEnabled;
    return true;
}

void CGameSA::SetupSpecialCharacters ( void )
{
    ModelInfo[1].MakePedModel ( "TRUTH" );
    ModelInfo[2].MakePedModel ( "MACCER" );
    //ModelInfo[190].MakePedModel ( "BARBARA" );
    //ModelInfo[191].MakePedModel ( "HELENA" );
    //ModelInfo[192].MakePedModel ( "MICHELLE" );
    //ModelInfo[193].MakePedModel ( "KATIE" );
    //ModelInfo[194].MakePedModel ( "MILLIE" );
    //ModelInfo[195].MakePedModel ( "DENISE" );
    ModelInfo[265].MakePedModel ( "TENPEN" );   
    ModelInfo[266].MakePedModel ( "PULASKI" );
    ModelInfo[267].MakePedModel ( "HERN" );
    ModelInfo[268].MakePedModel ( "DWAYNE" );
    ModelInfo[269].MakePedModel ( "SMOKE" );
    ModelInfo[270].MakePedModel ( "SWEET" );
    ModelInfo[271].MakePedModel ( "RYDER" );
    ModelInfo[272].MakePedModel ( "FORELLI" );
    ModelInfo[290].MakePedModel ( "ROSE" );
    ModelInfo[291].MakePedModel ( "PAUL" );
    ModelInfo[292].MakePedModel ( "CESAR" );
    ModelInfo[293].MakePedModel ( "OGLOC" );
    ModelInfo[294].MakePedModel ( "WUZIMU" );
    ModelInfo[295].MakePedModel ( "TORINO" );
    ModelInfo[296].MakePedModel ( "JIZZY" );
    ModelInfo[297].MakePedModel ( "MADDOGG" );
    ModelInfo[298].MakePedModel ( "CAT" );
    ModelInfo[299].MakePedModel ( "CLAUDE" );
    ModelInfo[300].MakePedModel ( "RYDER2" );
    ModelInfo[301].MakePedModel ( "RYDER3" );
    ModelInfo[302].MakePedModel ( "EMMET" );
    ModelInfo[303].MakePedModel ( "ANDRE" );
    ModelInfo[304].MakePedModel ( "KENDL" );
    ModelInfo[305].MakePedModel ( "JETHRO" );
    ModelInfo[306].MakePedModel ( "ZERO" );
    ModelInfo[307].MakePedModel ( "TBONE" );
    ModelInfo[308].MakePedModel ( "SINDACO" );
    ModelInfo[309].MakePedModel ( "JANITOR" );
    ModelInfo[310].MakePedModel ( "BBTHIN" );
    ModelInfo[311].MakePedModel ( "SMOKEV" );
    ModelInfo[312].MakePedModel ( "PSYCHO" );
    /* Hot-coffee only models
    ModelInfo[313].MakePedModel ( "GANGRL2" );
    ModelInfo[314].MakePedModel ( "MECGRL2" );
    ModelInfo[315].MakePedModel ( "GUNGRL2" );
    ModelInfo[316].MakePedModel ( "COPGRL2" );
    ModelInfo[317].MakePedModel ( "NURGRL2" );
    */
    // #7935 - Add peds to unused IDs so that servers can add mods to these skins
    ushort skins[] = {
        3, 4, 5, 6, 8, 42, 65, 74, 86, 119, 149, 208, 273, 289, 329, 340, 382, 383, 398, 399,
        612, 613, 614, 662, 663, 665, 666, 667, 668, 699, 793, 794, 795, 796, 797, 798, 799, 907,
        908, 909, 965, 999, 1194, 1195, 1196, 1197, 1198, 1199, 1200, 1201, 1202, 1203, 1204, 1205,
        1206, 1326, 1573, 1699, 2883, 2884, 3136, 3137, 3138, 3139, 3140, 3141, 3142, 3143, 3144,
        3145, 3146, 3147, 3148, 3149, 3150, 3151, 3152, 3153, 3154, 3155, 3156, 3157, 3158, 3159,
        3160, 3161, 3162, 3163, 3164, 3165, 3166, 3176, 3177, 3179, 3180, 3181, 3182, 3183,
        3184, 3185, 3186, 3188, 3189, 3190, 3191, 3192, 3194, 3195, 3196, 3197, 3198, 3199, 3200,
        3201, 3202, 3203, 3204, 3205, 3206, 3207, 3208, 3209, 3210, 3211, 3212, 3213, 3215,
        3216, 3217, 3218, 3219, 3220, 3222, 3223, 3224, 3225, 3226, 3227, 3228, 3229, 3230, 3231,
        3232, 3233, 3234, 3235, 3236, 3237, 3238, 3239, 3240, 3245, 3247, 3248, 3251, 3254,
        3266, 3348, 3349, 3416, 3429, 3610, 3611, 3784, 3870, 3871, 3883, 3889, 3974, 4542, 4543,
        4544, 4545, 4546, 4547, 4548, 4549, 4763, 4764, 4765, 4766, 4767, 4768, 4769, 4770, 4771,
        4772, 4773, 4774, 4775, 4776, 4777, 4778, 4779, 4780, 4781, 4782, 4783, 4784, 4785, 4786,
        4787, 4788, 4789, 4790, 4791, 4792, 4793, 4794, 4795, 4796, 4797, 4798, 4799, 4800, 4801,
        4802, 4803, 4804, 4805, 5090, 5104, 5376, 5377, 5378, 5379, 5380, 5381, 5382, 5383,
        5384, 5385, 5386, 5387, 5388, 5389
    };
    for ( ushort i = 0; i < sizeof ( skins ) / sizeof ( ushort ); i++ )
    {
        ModelInfo[ skins[i] ].MakePedModel ( "PSYCHO" ); 
    }
}

// Well, has it?
bool CGameSA::HasCreditScreenFadedOut ( void )
{
    BYTE ucAlpha = *(BYTE*)0xBAB320;
    bool bCreditScreenFadedOut = ( GetSystemState() >= 7 ) && ( ucAlpha < 6 );
    return bCreditScreenFadedOut;
}

// Ensure replaced/restored textures for models in the GTA map are correct
void CGameSA::FlushPendingRestreamIPL ( void )
{
    CModelInfoSA::StaticFlushPendingRestreamIPL ();
    m_pRenderWare->ResetStats ();
}

void CGameSA::GetShaderReplacementStats ( SShaderReplacementStats& outStats )
{
    m_pRenderWare->GetShaderReplacementStats ( outStats );
}

// Ensure models have the default lod distances
void CGameSA::ResetModelLodDistances ( void )
{
    CModelInfoSA::StaticResetLodDistances ();
}

void CGameSA::ResetAlphaTransparencies ( void )
{
    CModelInfoSA::StaticResetAlphaTransparencies ();
}

// Disable VSync by forcing what normally happends at the end of the loading screens
// Note #1: This causes the D3D device to be reset after the next frame
// Note #2: Some players do not need this to disable VSync. (Possibly because their video card driver settings override it somewhere)
void CGameSA::DisableVSync ( void )
{
    MemPutFast < BYTE > ( 0xBAB318, 0 );
}
CWeapon * CGameSA::CreateWeapon ( void )
{
    return new CWeaponSA ( new CWeaponSAInterface, NULL, WEAPONSLOT_MAX );
}

CWeaponStat * CGameSA::CreateWeaponStat ( eWeaponType weaponType, eWeaponSkill weaponSkill )
{
    return m_pWeaponStatsManager->CreateWeaponStatUnlisted ( weaponType, weaponSkill );
}

void CGameSA::OnPedContextChange ( CPed* pPedContext )
{
    m_pPedContext = pPedContext;
}

CPed* CGameSA::GetPedContext ( void )
{
    if ( !m_pPedContext )
        m_pPedContext = pGame->GetPools ()->GetPedFromRef ( (DWORD)1 );
    return m_pPedContext;
}
