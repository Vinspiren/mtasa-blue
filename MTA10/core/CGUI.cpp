/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        core/CGUI.cpp
*  PURPOSE:     Core graphical user interface container class
*  DEVELOPERS:  Cecill Etheredge <ijsf@gmx.net>
*               Christian Myhre Lundheim <>
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#include "StdInc.h"
#include <game/CGame.h>

using std::string;

template<> CLocalGUI * CSingleton < CLocalGUI >::m_pSingleton = NULL;

#ifndef HIWORD
    #define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))
#endif
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))

const char* DEFAULT_SKIN_NAME = "Default"; // TODO: Change to whatever the default skin is if it changes

CLocalGUI::CLocalGUI ( void )
{
    m_pConsole = NULL;
    m_pMainMenu = NULL;
    m_pChat = NULL;
    m_pDebugView = NULL;

    m_bForceCursorVisible = false;
    m_bChatboxVisible = true;
    m_pDebugViewVisible = false;
    m_bGUIHasInput = false;
    m_uiActiveCompositionSize = 0;

    m_pVersionUpdater = GetVersionUpdater ();

    m_LastSettingsRevision = -1;
    m_LocaleChangeCounter = 0;
}


CLocalGUI::~CLocalGUI ( void )
{
    // Destroy all GUI elements
    DestroyObjects ();
    // This is needed after the local gui is deleted for config saving
    //delete m_pVersionUpdater;
}

void CLocalGUI::SetSkin( const char* szName )
{
    bool guiWasLoaded = m_pMainMenu != NULL;
    if(guiWasLoaded)
        DestroyWindows();

    std::string error;

    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    try 
    {
        pGUI->SetSkin(szName);
        m_LastSkinName = szName;
    }
    catch (...)
    {
        try
        {
            pGUI->SetSkin(DEFAULT_SKIN_NAME);

            error = "The skin '" + std::string(szName) + "' that you have selected could not be loaded. MTA is now using the default skin instead.";
            
            CVARS_SET("current_skin", std::string(DEFAULT_SKIN_NAME));    
            m_LastSkinName = DEFAULT_SKIN_NAME;
        }
        catch(...)
        {
            // Even the default skin doesn't work, so give up
            MessageBoxUTF8 ( 0, _("The skin you selected could not be loaded, and the default skin also could not be loaded, please reinstall MTA."), _("Error")+_E("CC51"), MB_OK | MB_ICONERROR | MB_TOPMOST );
            TerminateProcess ( GetCurrentProcess (), 9 );
        }
    }

    CClientVariables* cvars = CCore::GetSingleton().GetCVars();
    m_LastSettingsRevision = cvars->GetRevision();

    if(guiWasLoaded)
        CreateWindows(guiWasLoaded);

    if(CCore::GetSingleton().GetConsole() && !error.empty())
        CCore::GetSingleton().GetConsole()->Echo(error.c_str());

}


void CLocalGUI::ChangeLocale( const char* szName )
{
    bool guiWasLoaded = m_pMainMenu != NULL;
    assert(guiWasLoaded);
    if(guiWasLoaded)
        DestroyWindows();

    CClientVariables* cvars = CCore::GetSingleton().GetCVars();
    m_LastSettingsRevision = cvars->GetRevision();

    // Don't delete old Localization as it crashes
    g_pLocalization = new CLocalization;
    m_LastLocaleName = szName;

    if(guiWasLoaded)
        CreateWindows(guiWasLoaded);
}


void CLocalGUI::CreateWindows ( bool bGameIsAlreadyLoaded )
{
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    // Create chatbox
    m_pChat = new CChat ( pGUI, CVector2D ( 0.0125f, 0.015f ) );
    m_pChat->SetVisible ( false );

    // Create the debug view
    m_pDebugView = new CDebugView ( pGUI, CVector2D ( 0.23f, 0.785f ) );
    m_pDebugView->SetVisible ( false );

    // Create the overlayed version labels
    CVector2D ScreenSize = pGUI->GetResolution ();
    SString strText = "MTA:SA " MTA_DM_BUILDTAG_SHORT;
    if ( _NETCODE_VERSION_BRANCH_ID != 0x04 )
        strText += SString( " (%X)", _NETCODE_VERSION_BRANCH_ID );
    m_pLabelVersionTag = reinterpret_cast < CGUILabel* > ( pGUI->CreateLabel ( strText ) );
    m_pLabelVersionTag->SetSize ( CVector2D ( m_pLabelVersionTag->GetTextExtent() + 5, 18 ) );
    m_pLabelVersionTag->SetPosition ( CVector2D ( ScreenSize.fX - m_pLabelVersionTag->GetTextExtent() - 5, ScreenSize.fY - 15 ) );
    m_pLabelVersionTag->SetAlpha ( 0.5f );
    m_pLabelVersionTag->SetTextColor ( 255, 255, 255 );
    m_pLabelVersionTag->SetZOrderingEnabled ( false );
    m_pLabelVersionTag->MoveToBack ();
    m_pLabelVersionTag->SetVisible ( false );

    // Create mainmenu
    m_pMainMenu = new CMainMenu ( pGUI );
    m_pMainMenu->SetVisible ( bGameIsAlreadyLoaded, !bGameIsAlreadyLoaded, false );

    // Create console
    m_pConsole = new CConsole ( pGUI );
    m_pConsole->SetVisible ( false );

    // Create community registration window
    m_CommunityRegistration.CreateWindows ();
    m_CommunityRegistration.SetVisible ( false );

    // Create our news headlines if we're already ingame
    if ( bGameIsAlreadyLoaded )
        m_pMainMenu->GetNewsBrowser()->CreateHeadlines();
}


void CLocalGUI::CreateObjects ( IUnknown* pDevice )
{
    // Store the GUI manager pointer and create the GUI classes
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    // Set the CEGUI skin to whatever the user has selected
    SString currentSkinName;
    CClientVariables* cvars = CCore::GetSingleton().GetCVars();
    cvars->Get("current_skin", currentSkinName);
    if(currentSkinName.empty())
    {
        currentSkinName = DEFAULT_SKIN_NAME; 
        CVARS_SET("current_skin", currentSkinName);
    }

    SetSkin(currentSkinName);

    CreateWindows ( false );
}


void CLocalGUI::DestroyWindows ( void )
{
    SAFE_DELETE ( m_pLabelVersionTag );
    SAFE_DELETE ( m_pConsole );
    SAFE_DELETE ( m_pMainMenu );
    SAFE_DELETE ( m_pChat );
    SAFE_DELETE ( m_pDebugView );
}


void CLocalGUI::DestroyObjects ( void )
{
    DestroyWindows ();

    // Destroy and NULL all elements
    SAFE_DELETE ( m_pLabelVersionTag );
}


void CLocalGUI::DoPulse ( void )
{
    m_CommunityRegistration.DoPulse ();
    m_pVersionUpdater->DoPulse ();

    CClientVariables* cvars = CCore::GetSingleton().GetCVars();
    if(cvars->GetRevision() != m_LastSettingsRevision)
    {
        m_LastSettingsRevision = cvars->GetRevision();

        //
        // Check for skin change
        //
        SString currentSkinName;
        cvars->Get("current_skin", currentSkinName);

        if(currentSkinName != m_LastSkinName)
        {
            if ( !CCore::GetSingleton ().GetModManager()->IsLoaded() )
                SetSkin(currentSkinName);
            else
            {
                CCore::GetSingleton ().GetConsole()->Printf ( "Please disconnect before changing skin" );
                cvars->Set("current_skin", m_LastSkinName );
            }
        }

        //
        // Check for locale change
        //
        SString currentLocaleName;
        cvars->Get( "locale", currentLocaleName );

        if ( m_LastLocaleName.empty() )
        {
            m_LastLocaleName = currentLocaleName;
        }
        if ( currentLocaleName != m_LastLocaleName )
        {
            m_LocaleChangeCounter++;
            if ( m_LocaleChangeCounter < 5 )
            {
                // Do GUI stuff for first 5 frames
                // Force pulse next time
                m_LastSettingsRevision = cvars->GetRevision() - 1;

                if ( m_LocaleChangeCounter == 2 )
                    CCore::GetSingleton ().ShowMessageBox ( _E("CC99"), ("PLEASE WAIT...................."), MB_ICON_INFO );
            }
            else
            {
                // Do actual local change
                m_LocaleChangeCounter = 0;
                CCore::GetSingleton ().RemoveMessageBox();
    
                if ( !CCore::GetSingleton ().GetModManager()->IsLoaded() )
                    ChangeLocale( currentLocaleName );
                else
                {
                    CCore::GetSingleton ().GetConsole()->Printf ( "Please disconnect before changing language" );
                    cvars->Set("locale", m_LastLocaleName );
                }
            }
        }
    }
}


void CLocalGUI::Draw ( void )
{
    // Get the game interface
    CGame* pGame = CCore::GetSingleton ().GetGame ();
    eSystemState SystemState = pGame->GetSystemState ();
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    // Update mainmenu stuff
    m_pMainMenu->Update ();

    // Make sure our version labels are always visible
    static short WaitForMenu = 0;

    // Cope with early finish
    if ( pGame->HasCreditScreenFadedOut () )
        WaitForMenu = 250;

    if ( SystemState == 7 || SystemState == 9 ) {
        if ( WaitForMenu < 250 ) {
            WaitForMenu++;
        } else {
            m_pLabelVersionTag->SetVisible ( true );
            if ( MTASA_VERSION_TYPE < VERSION_TYPE_RELEASE )
                m_pLabelVersionTag->SetAlwaysOnTop ( true );
        }
    }

    // If we're ingame, make sure the chatbox is drawn
    bool bChatVisible = ( SystemState == 9 /* GS_INGAME */ && m_pMainMenu->GetIsIngame () && m_bChatboxVisible && !CCore::GetSingleton ().IsOfflineMod() );
    if ( m_pChat->IsVisible () != bChatVisible )
        m_pChat->SetVisible ( bChatVisible );
    bool bDebugVisible = ( SystemState == 9 /* GS_INGAME */ && m_pMainMenu->GetIsIngame () && m_pDebugViewVisible && !CCore::GetSingleton ().IsOfflineMod() );
    if ( m_pDebugView->IsVisible () != bDebugVisible )
        m_pDebugView->SetVisible ( bDebugVisible );

    // Make sure the cursor is displayed only when needed
    UpdateCursor ();

    // Draw the chat
    m_pChat->Draw ( true );
    // Draw the debugger
    m_pDebugView->Draw ( false );

    // If we're not at the loadingscreen
    static bool bDelayedFrame = false;
    if ( SystemState != 8 || !bDelayedFrame /* GS_INIT_PLAYING_GAME */ )
    {
        // If we have a GUI manager, draw the GUI
        if ( pGUI )
        {
            pGUI->Draw ( );
        }

        // If the system state was 8, make sure we don't do another delayed frame
        if ( SystemState == 8 )
        {
            bDelayedFrame = true;
        }
    }
}


void CLocalGUI::Invalidate ( void )
{
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    // Invalidate the GUI
    if ( pGUI )
    {
        pGUI->Invalidate ( );
    }
}


void CLocalGUI::Restore ( void )
{
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    if ( pGUI )
    {
        // Restore the GUI
        pGUI->Restore ();
    }
}

void CLocalGUI::DrawMouseCursor ( void )
{
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    pGUI->DrawMouseCursor ();
}


CConsole* CLocalGUI::GetConsole ( void )
{
    return m_pConsole;
}


void CLocalGUI::SetConsoleVisible ( bool bVisible )
{
    if ( m_pConsole )
    {
        // Only allow the console if the chat doesn't take input
        if ( m_pChat->IsInputVisible () )
            bVisible = false;
        
        // Set the visible state
        m_pConsole->SetVisible ( bVisible );

        CGUI* pGUI = CCore::GetSingleton ().GetGUI ();
        if ( bVisible )
	        pGUI->SetCursorAlpha ( 1.0f );
        else
	        pGUI->SetCursorAlpha ( pGUI->GetCurrentServerCursorAlpha () );
    }
}


bool CLocalGUI::IsConsoleVisible ( void )
{
    if ( m_pConsole )
    {
        return m_pConsole->IsVisible ();
    }
    return false;
}


void CLocalGUI::EchoConsole ( const char* szText )
{
    if ( m_pConsole )
    {
        m_pConsole->Echo ( szText );
    }
}


CMainMenu* CLocalGUI::GetMainMenu ( void )
{
    return m_pMainMenu;
}


void CLocalGUI::SetMainMenuVisible ( bool bVisible )
{
    if ( m_pMainMenu )
    {
        // This code installs the original CCore input handlers when the ingame menu
        // is shown, and restores the mod input handlers when the menu is hidden again.
        // This is needed for things like pressing escape when changing a key bind

        m_pMainMenu->SetVisible ( bVisible );

        CGUI* pGUI = CCore::GetSingleton ().GetGUI ();
        if ( bVisible )
        {
            pGUI->SelectInputHandlers ( INPUT_CORE );
        }
        else
        {
            pGUI->SelectInputHandlers ( INPUT_MOD );
        }

        if ( bVisible )
            pGUI->SetCursorAlpha ( 1.0f );
        else
            pGUI->SetCursorAlpha ( pGUI->GetCurrentServerCursorAlpha () );
    }
}


bool CLocalGUI::IsMainMenuVisible ( void )
{
    if ( m_pMainMenu )
    {
        return m_pMainMenu->IsVisible ();
    }
    return false;
}

CChat* CLocalGUI::GetChat ( void )
{
    return m_pChat;
}


CDebugView* CLocalGUI::GetDebugView ( void )
{
    return m_pDebugView;
}

void CLocalGUI::SetChatBoxVisible ( bool bVisible )
{
    if ( m_pChat )
    {
        if ( m_pChat->IsVisible () != bVisible )    
            m_pChat->SetVisible ( bVisible );
        m_bChatboxVisible = bVisible;
    }
}

void CLocalGUI::SetDebugViewVisible ( bool bVisible )
{
    if ( m_pDebugView )
    {
        if ( m_pDebugView->IsVisible () != bVisible )
            m_pDebugView->SetVisible ( bVisible );
        m_pDebugViewVisible = bVisible;
    }
}

bool CLocalGUI::IsChatBoxVisible ( void )
{
    if ( m_pChat )
    {
        return m_bChatboxVisible;
    }
    return false;
}

bool CLocalGUI::IsDebugViewVisible ( void )
{
    if ( m_pDebugView )
    {
        return m_pDebugViewVisible;
    }
    return false;
}


void CLocalGUI::SetChatBoxInputEnabled ( bool bInputEnabled )
{
    if ( m_pChat )
    {
        m_pChat->SetInputVisible ( bInputEnabled );
    }
}


bool CLocalGUI::IsChatBoxInputEnabled ( void )
{
    if ( m_pChat )
    {
        return m_pChat->IsInputVisible () && m_bChatboxVisible;
    }
    return false;
}


void CLocalGUI::EchoChat ( const char* szText, bool bColorCoded )
{
    if ( m_pChat )
    {
        m_pChat->Output ( szText, bColorCoded );
    }
}

bool CLocalGUI::IsWebRequestGUIVisible ()
{
    auto pWebCore = g_pCore->GetWebCore ();
    if ( pWebCore )
    {
        return pWebCore->IsRequestsGUIVisible ();
    }
    return false;
}

void CLocalGUI::EchoDebug ( const char* szText )
{
    if ( m_pDebugView )
    {
        m_pDebugView->Output ( szText, false );
    }
}

bool CLocalGUI::ProcessMessage ( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    // If we have the focus, we handle the message
    if ( InputGoesToGUI () )
    {
        // Pass the message to the GUI manager
        // ACHTUNG: fix the CEGUI ones!
        switch ( uMsg )
        {
            case WM_MOUSEWHEEL:
                if ( GET_WHEEL_DELTA_WPARAM ( wParam ) > 0 )  
                    pGUI->ProcessMouseInput ( CGUI_MI_MOUSEWHEEL, 1, NULL );
                else
                    pGUI->ProcessMouseInput ( CGUI_MI_MOUSEWHEEL, 0, NULL );
                return true;

            case WM_MOUSEMOVE:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEPOS, LOWORD ( lParam ), HIWORD ( lParam ) );
                return true;

            case WM_LBUTTONDOWN:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEDOWN, 0, 0, LeftButton );
                return true;

            case WM_RBUTTONDOWN:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEDOWN, 0, 0, RightButton );
                return true;

            case WM_MBUTTONDOWN:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEDOWN, 0, 0, MiddleButton );
                return true;

            case WM_LBUTTONUP:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEUP, 0, 0, LeftButton );
                return true;

            case WM_RBUTTONUP:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEUP, 0, 0, RightButton );
                return true;

            case WM_MBUTTONUP:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEUP, 0, 0, MiddleButton );
                return true;
#ifdef WM_XBUTTONDOWN
            case WM_XBUTTONDOWN:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEDOWN, 0, 0, X1Button );
                return true;

            case WM_XBUTTONUP:
                pGUI->ProcessMouseInput ( CGUI_MI_MOUSEUP, 0, 0, X1Button );
                return true;
#endif
            case WM_KEYDOWN:
            {
                DWORD dwTemp = TranslateScanCodeToGUIKey ( wParam );
                if ( dwTemp > 0 )
                {
                    pGUI->ProcessKeyboardInput ( dwTemp, true );
                    return true;
                }

                return true;
            }

            case WM_KEYUP:
            {
                DWORD dwTemp = TranslateScanCodeToGUIKey ( wParam );
                if ( dwTemp > 0 )
                {
                    pGUI->ProcessKeyboardInput ( dwTemp, false );
                }

                return false;
            }

            case WM_IME_COMPOSITION:
            {
                if ( lParam & GCS_RESULTSTR )
                {
                    HIMC himc = ImmGetContext ( hwnd );

                    // Get composition result
                    ushort buffer[256];
                    LONG numBytes = ImmGetCompositionStringW ( himc, GCS_RESULTSTR, buffer, sizeof ( buffer ) - 2 );
                    int iNumCharacters = numBytes / sizeof ( ushort );

                    // Erase output from previous composition state
                    for ( int i = 0 ; i < m_uiActiveCompositionSize ; i++ )
                    {
                        pGUI->ProcessCharacter ( '\x08' );
                        pGUI->ProcessKeyboardInput ( 14, true );
                    }

                    // Output composition result
                    for ( int i = 0 ; i < iNumCharacters ; i++ )
                        if ( buffer[i] )
                            pGUI->ProcessCharacter ( buffer[i] );

                    ImmReleaseContext ( hwnd, himc );

                    m_uiActiveCompositionSize = 0;
                }
                else if( lParam & GCS_COMPSTR ) 
                {
                    HIMC himc = ImmGetContext ( hwnd );

                    // Get composition state
                    ushort buffer[256];
                    LONG numBytes = ImmGetCompositionStringW ( himc, GCS_COMPSTR, buffer, sizeof ( buffer ) - 2 );
                    int iNumCharacters = numBytes / sizeof ( ushort );

                    // Erase output from previous composition state
                    for ( int i = 0 ; i < m_uiActiveCompositionSize ; i++ )
                    {
                        pGUI->ProcessCharacter ( '\x08' );
                        pGUI->ProcessKeyboardInput ( 14, true );
                    }

                    // Output new composition state
                    for ( int i = 0 ; i < iNumCharacters ; i++ )
                        if ( buffer[i] )
                            pGUI->ProcessCharacter ( buffer[i] );

                    ImmReleaseContext ( hwnd, himc );

                    m_uiActiveCompositionSize = iNumCharacters;
                }
            }
            break;

            case WM_IME_CHAR:
                return true;
            case WM_IME_KEYDOWN:
            {
                // Handle space/return seperately in this case
                if ( wParam == VK_SPACE   )
                    pGUI->ProcessCharacter ( MapVirtualKey( wParam, MAPVK_VK_TO_CHAR ) );

                DWORD dwTemp = TranslateScanCodeToGUIKey ( wParam );
                if ( dwTemp > 0 )
                    pGUI->ProcessKeyboardInput ( dwTemp, true );
            }
            break;

            case WM_CHAR:
            {
                pGUI->ProcessCharacter ( wParam );
                return true;
            }
            break;
        }
    }

    // The event wasn't handled
    return false;
}


bool CLocalGUI::InputGoesToGUI ( void )
{
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();
    if ( !pGUI ) return false;

    // Here we're supposed to check if things like menues are up, console is up or the chatbox is expecting input
    // If the console is visible OR the chat is expecting input OR the mainmenu is visible
    return ( IsConsoleVisible () || IsMainMenuVisible () || IsChatBoxInputEnabled () || m_bForceCursorVisible || pGUI->GetGUIInputEnabled () || !CCore::GetSingleton ().IsFocused () || IsWebRequestGUIVisible () );
}


void CLocalGUI::ForceCursorVisible ( bool bVisible )
{
    m_bForceCursorVisible = bVisible;
}


void CLocalGUI::UpdateCursor ( void )
{
    CGUI* pGUI = CCore::GetSingleton ().GetGUI ();

    static DWORD dwWidth = CDirect3DData::GetSingleton().GetViewportWidth();
    static DWORD dwHeight = CDirect3DData::GetSingleton().GetViewportHeight();
    static bool bFirstRun = true;

    if ( bFirstRun )
    {
        m_StoredMousePosition.x = dwWidth / 2;
        m_StoredMousePosition.y = dwHeight / 2;
        bFirstRun = false;
    }
    // Called in each frame to make sure the mouse is only visible when a GUI control that uses the
    // mouse requires it.
    if ( InputGoesToGUI () )
    {
        // We didn't have focus last pulse?
        if ( !m_bGUIHasInput )
        {
            /* Store if the controller was enabled or not and disable it
            CCore::GetSingleton ().GetGame ()->GetPad ()->Disable ( true );
            CCore::GetSingleton ().GetGame ()->GetPad ()->Clear ();*/

            // Restore the mouse cursor to its old position
            SetCursorPos ( m_StoredMousePosition.x, m_StoredMousePosition.y );

            // Enable our mouse cursor
            CSetCursorPosHook::GetSingleton ( ).DisableSetCursorPos ();
            pGUI->SetCursorEnabled ( true );

            m_bGUIHasInput = true;
        }
    }
    else
    {
        // We had focus last frame?
        if ( m_bGUIHasInput )
        {
            /* Restore the controller state
            CCore::GetSingleton ().GetGame ()->GetPad ()->Disable ( false );
            CCore::GetSingleton ().GetGame ()->GetPad ()->Clear ();*/

            // Set the mouse back to the center of the screen (to prevent the game from reacting to its movement)
            SetCursorPos ( dwWidth / 2, dwHeight / 2 );

            // Disable our mouse cursor
            CSetCursorPosHook::GetSingleton ( ).EnableSetCursorPos ();
            pGUI->SetCursorEnabled ( false );

            // Clear any held system keys
            pGUI->ClearSystemKeys ();

            m_bGUIHasInput = false;
        }
    }
}


DWORD CLocalGUI::TranslateScanCodeToGUIKey ( DWORD dwCharacter )
{
    // The following switch case is necessary to convert input WM_KEY* messages
    // to corresponding DirectInput key messages.  CEGUI needs these.

    switch ( dwCharacter )
    {
        case VK_HOME:       return DIK_HOME;
        case VK_END:        return DIK_END;
        case VK_RETURN:     return DIK_RETURN;
        case VK_TAB:        return DIK_TAB;
        case VK_BACK:       return DIK_BACK;
        case VK_LEFT:       return DIK_LEFTARROW;
        case VK_RIGHT:      return DIK_RIGHTARROW;
        case VK_UP:         return DIK_UPARROW;
        case VK_DOWN:       return DIK_DOWNARROW;
        case VK_LSHIFT:     return DIK_LSHIFT;
        case VK_RSHIFT:     return DIK_RSHIFT;
        case VK_SHIFT:      return DIK_LSHIFT;
        case VK_CONTROL:    return DIK_LCONTROL;
        case VK_DELETE:     return DIK_DELETE;
        case 0x56:          return DIK_V;           // V
        case 0x43:          return DIK_C;           // C
        case 0x58:          return DIK_X;           // X
        case 0x41:          return DIK_A;           // A
        default:            return 0;
    }
}

void CLocalGUI::SetCursorPos ( int iX, int iY, bool bForce )
{
    // Update the stored position
    m_StoredMousePosition.x = iX;
    m_StoredMousePosition.y = iY;

    // Apply the position
    if ( bForce )
        CSetCursorPosHook::GetSingleton ().CallSetCursorPos ( iX, iY );
    else
        ::SetCursorPos ( iX, iY );

}