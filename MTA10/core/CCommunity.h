/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        core/CCommunity.h
*  PURPOSE:     Header file for community connector class
*  DEVELOPERS:  Cecill Etheredge <ijsf@gmx.net>
*               Sebas Lamers <sebasdevelopment@gmx.com>
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#ifndef __CCOMMUNITY_H
#define __CCOMMUNITY_H

#include <core/CCommunityInterface.h>

typedef void (*VERIFICATIONCALLBACK) ( bool, char*, void* obj );

#define VERIFICATION_DELAY              9000
#define VERIFICATION_URL                "http://community.mtasa.com/core/1.1/client.php"

enum eVerificationResult
{
    VERIFY_ERROR_UNEXPECTED = 0,
    VERIFY_ERROR_SUCCESS,
    VERIFY_ERROR_VERIFICATION_FAILED,
    VERIFY_ERROR_COMPUTER_USED,
    VERIFY_ERROR_SERIALS_MAX,
    VERIFY_ERROR_INVALID_ACCOUNT,
    VERIFY_ERROR_ACCOUNT_USED,
    VERIFY_ERROR_INVALID,
    VERIFY_ERROR_ACCOUNT_BANNED,
    VERIFY_ERROR_SERIAL_BANNED,
    VERIFY_ERROR_CURL,
};

class CCommunity : public CCommunityInterface, public CSingleton < CCommunity >
{
    friend class CSettings;
    friend class CCommunityLogin;

public:
                                CCommunity          ( void );
                                ~CCommunity         ( void );

    void                        Initialize          ( void );
    void                        GetUsername         ( std::string & strUsername )   { strUsername = m_strUsername; };
    void                        GetPassword         ( std::string & strPassword )   { strPassword = m_strPassword; };

    bool                        IsLoggedIn          ( void )                        { return m_bLoggedIn; };

    void                        DoPulse             ( void );

protected:
    void                        SetUsername         ( const std::string& strUsername )     { m_strUsername = strUsername; };
    void                        SetPassword         ( const std::string& strPassword )     { m_strPassword = strPassword; };
    
    void                        Login               ( VERIFICATIONCALLBACK pCallBack = NULL, void* pObject = NULL );
    void                        Logout              ( void );

    CNetHTTPDownloadManagerInterface* GetHTTP       ( void );
    static void                 StaticDownloadFinished ( char* pCompletedData, size_t completedLength, void *pObj, bool bSuccess, int iErrorCode );
    void                        DownloadFailed      ( int iErrorCode );
    void                        DownloadSuccess     ( char* pCompletedData, size_t completedLength );

private:
    std::string                 m_strUsername;
    std::string                 m_strPassword;

    bool                        m_bLoggedIn;
    unsigned long               m_ulStartTime;

    // Callback variables
    VERIFICATIONCALLBACK        m_pCallback;
    void *                      m_pVerificationObject;
};

#endif
