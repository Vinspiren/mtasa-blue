/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        mods/deathmatch/logic/CHTTPD.cpp
*  PURPOSE:     Built-in HTTP webserver class
*  DEVELOPERS:  Ed Lyons <>
*               Christian Myhre Lundheim <>
*               Cecill Etheredge <>
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#include "StdInc.h"
#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>

extern CGame * g_pGame;

CHTTPD::CHTTPD ( void )
    : m_BruteForceProtect( 4, 30000, 60000 * 5 )     // Max of 4 attempts per 30 seconds, then 5 minute ignore
    , m_HttpDosProtect( 0, 0, 0 )
{
    m_resource = NULL;
    m_server = NULL;
    m_bStartedServer = false;

    m_pGuestAccount = new CAccount ( g_pGame->GetAccountManager (), false, HTTP_GUEST_ACCOUNT_NAME );

    m_HttpDosProtect = CConnectHistory ( g_pGame->GetConfig ()->GetHTTPDosThreshold (), 10000, 60000 * 1 );     // Max of 'n' connections per 10 seconds, then 1 minute ignore

    std::vector< SString > excludeList;
    g_pGame->GetConfig ()->GetHTTPDosExclude ().Split( ",", excludeList );
    m_HttpDosExcludeMap = std::set < SString > ( excludeList.begin(), excludeList.end() );
}


CHTTPD::~CHTTPD ()
{
    StopHTTPD ();
}


bool CHTTPD::StopHTTPD ( void )
{
    // Stop the server if we started it
    if ( m_bStartedServer )
    {
        // Stop the server
        StopServer ();
        m_bStartedServer = false;
        return true;
    }
    return false;
}


bool CHTTPD::StartHTTPD ( const char* szIP, unsigned int port )
{
    bool bResult = false;

    // Server not already started?
    if ( !m_bStartedServer )
    {
        EHSServerParameters parameters;

        char szPort[10];
        itoa ( port, szPort, 10 );
        parameters[ "port" ] = szPort;

        if ( szIP && szIP[0] )
        {
            // Convert the dotted ip to a long
            long lIP = inet_addr ( szIP );
            parameters[ "bindip" ] = lIP;
        }
        else
        {
            // Bind to default;
            parameters[ "bindip" ] = (long) INADDR_ANY;
        }

        parameters[ "mode" ] = "threadpool";        // or "singlethreaded"/"threadpool"
        parameters[ "threadcount" ] = g_pGame->GetConfig ()->GetHTTPThreadCount ();

        bResult = ( StartServer ( parameters ) == STARTSERVER_SUCCESS );
        m_bStartedServer = true;
    }

    return bResult;
}

// Called from worker thread.
// Do some stuff before allowing EHS to do the proper routing
HttpResponse * CHTTPD::RouteRequest ( HttpRequest * ipoHttpRequest )
{
    if ( !g_pGame->IsServerFullyUp () )
    {
        // create an HttpRespose object for the message
        HttpResponse* poHttpResponse = new HttpResponse ( ipoHttpRequest->m_nRequestId, ipoHttpRequest->m_poSourceEHSConnection );
        SStringX strWait ( "The server is not ready. Please try again in a minute." );
        poHttpResponse->SetBody ( strWait.c_str (), strWait.size () );
        poHttpResponse->m_nResponseCode = HTTPRESPONSECODE_200_OK;
        return poHttpResponse;
    }

    // Sync with main thread before routing (to a resource)
    g_pGame->Lock();
    HttpResponse* poHttpResponse = EHS::RouteRequest( ipoHttpRequest );
    g_pGame->Unlock();

    return poHttpResponse;
}

// Called from worker thread. g_pGame->Lock() has already been called.
// creates a page based on user input -- either displays data from
//   form or presents a form for users to submit data.
ResponseCode CHTTPD::HandleRequest ( HttpRequest * ipoHttpRequest,
                                         HttpResponse * ipoHttpResponse )
{
    // Check if server verification was requested
    auto challenge = ipoHttpRequest->oRequestHeaders["crypto_challenge"];
    if ( ipoHttpRequest->sUri == "/get_verification_key_code" && challenge != "" )
    {
        auto path = g_pServerInterface->GetModManager ()->GetAbsolutePath ( "verify.key" );
        SString encodedPublicKey;
        SharedUtil::FileLoad ( path, encodedPublicKey, 392 );

        using namespace CryptoPP;

        try
        {
            // Load public RSA key from disk
            RSA::PublicKey publicKey;
            std::string base64Data;
            Base64::decode ( encodedPublicKey, base64Data );
            StringSource stringSource ( base64Data, true );
            publicKey.Load ( stringSource );

            // Launch encryptor and encrypt
            RSAES_OAEP_SHA_Encryptor encryptor ( publicKey );
            SecByteBlock cipherText ( encryptor.CiphertextLength ( challenge.size () ) );
            AutoSeededRandomPool rng;
            encryptor.Encrypt ( rng, (const byte*) challenge.data (), challenge.size (), cipherText.begin () );

            if ( !cipherText.empty () )
            {
                ipoHttpResponse->SetBody ( (const char*)cipherText.BytePtr (), cipherText.SizeInBytes () );
                return HTTPRESPONSECODE_200_OK;
            }
            else
                CLogger::LogPrintf ( LOGLEVEL_MEDIUM, "ERROR: Empty crypto challenge was passed during verification\n" );
        }
        catch ( const std::exception& )
        {
            CLogger::LogPrintf ( LOGLEVEL_MEDIUM, "ERROR: Invalid verify.key keyfile\n" );
        }

        ipoHttpResponse->SetBody ( "", 0 );
        return HTTPRESPONSECODE_401_UNAUTHORIZED;
    }

    CAccount * account = CheckAuthentication ( ipoHttpRequest );

    if ( account )
    {
        if ( !m_strDefaultResourceName.empty () )
        {
            SString strWelcome ( "<a href='/%s/'>This is the page you want</a>", m_strDefaultResourceName.c_str () );
            ipoHttpResponse->SetBody ( strWelcome.c_str (), strWelcome.size () );
            SString strNewURL ( "http://%s/%s/", ipoHttpRequest->oRequestHeaders["host"].c_str(), m_strDefaultResourceName.c_str () );
            ipoHttpResponse->oResponseHeaders["location"] = strNewURL.c_str ();
            return HTTPRESPONSECODE_302_FOUND;
        }
    }

    SString strWelcome ( "You haven't set a default resource in your configuration file. You can either do this or visit http://%s/<i>resourcename</i>/ to see a specific resource.<br/><br/>Alternatively, the server may be still starting up, if so, please try again in a minute.", ipoHttpRequest->oRequestHeaders["host"].c_str() );
    ipoHttpResponse->SetBody ( strWelcome.c_str (), strWelcome.size () );
    return HTTPRESPONSECODE_200_OK;
}

ResponseCode CHTTPD::RequestLogin ( HttpResponse * ipoHttpResponse )
{
    const char* szAuthenticateMessage = "Access denied, please login";
    ipoHttpResponse->SetBody ( szAuthenticateMessage, strlen(szAuthenticateMessage) );
    SString strName ( "Basic realm=\"%s\"", g_pGame->GetConfig ()->GetServerName ().c_str () );
    ipoHttpResponse->oResponseHeaders [ "WWW-Authenticate" ] = strName.c_str ();
    return HTTPRESPONSECODE_401_UNAUTHORIZED;
}

CAccount * CHTTPD::CheckAuthentication ( HttpRequest * ipoHttpRequest )
{
    string authorization = ipoHttpRequest->oRequestHeaders["authorization"];
    if ( authorization.length() > 6 )
    {
        if ( authorization.substr(0,6) == "Basic " )
        {
            // Basic auth
            std::string strAuthDecoded;
            Base64::decode ( authorization.substr(6), strAuthDecoded );

            string authName;
            string authPassword;
            for ( size_t i = 0; i < strAuthDecoded.length(); i++ )
            {
                if ( strAuthDecoded.substr(i, 1) == ":" )
                {
                    authName = strAuthDecoded.substr(0,i);
                    authPassword = strAuthDecoded.substr(i+1);
                    break;
                }
            }

            if ( m_BruteForceProtect.IsFlooding ( ipoHttpRequest->GetAddress ().c_str () ) )
            {
                CLogger::AuthPrintf ( "HTTP: Ignoring login attempt for user '%s' from %s\n", authName.c_str () , ipoHttpRequest->GetAddress ().c_str () );
                return m_pGuestAccount;
            }

            CAccount * account = g_pGame->GetAccountManager()->Get ( authName.c_str());
            if ( account )
            {
                // Check that the password is right
                if ( account->IsPassword ( authPassword.c_str () ) )
                {
                    // Check that it isn't the Console account
                    std::string strAccountName = account->GetName ();
                    if ( strAccountName.compare ( CONSOLE_ACCOUNT_NAME ) != 0 )
                    {
                        // Handle initial login logging
                        if ( m_LoggedInMap.find ( authName ) == m_LoggedInMap.end () )
                            CLogger::AuthPrintf ( "HTTP: '%s' entered correct password from %s\n", authName.c_str () , ipoHttpRequest->GetAddress ().c_str () );
                        m_LoggedInMap[authName] = GetTickCount64_ ();
                        // @@@@@ Check they can access HTTP
                        return account;
                    }
                }
            }
            if ( authName.length () > 0 )
            {
                m_BruteForceProtect.AddConnect ( ipoHttpRequest->GetAddress ().c_str () );
                CLogger::AuthPrintf ( "HTTP: Failed login attempt for user '%s' from %s\n", authName.c_str () , ipoHttpRequest->GetAddress ().c_str () );
            }
        }
    }
    return m_pGuestAccount;
}

void CHTTPD::HttpPulse ( void )
{
    // Prevent more than one thread running this at once
    static int iBusy = 0;
    if ( ++iBusy > 1 )
    {
        iBusy--;
        return;
    }

    long long llExpireTime = GetTickCount64_ () - 1000 * 60 * 5;    // 5 minute timeout

    map < string, long long > :: iterator iter = m_LoggedInMap.begin ();
    while ( iter != m_LoggedInMap.end () )
    {
        // Remove if too long since last request
        if ( iter->second < llExpireTime )
        {
            g_pGame->Lock(); // get the mutex (blocking)
            CLogger::AuthPrintf ( "HTTP: '%s' no longer connected\n", iter->first.c_str () );
            m_LoggedInMap.erase ( iter++ );
            g_pGame->Unlock();
        }
        else
            iter++;
    }

    iBusy--;
}

//
// Do DoS check here
//
bool CHTTPD::ShouldAllowConnection ( const char * szAddress )
{
    if ( MapContains( m_HttpDosExcludeMap, szAddress ) )
        return true;

    if ( m_HttpDosProtect.IsFlooding ( szAddress ) )
        return false;

    m_HttpDosProtect.AddConnect ( szAddress );

    if ( m_HttpDosProtect.IsFlooding ( szAddress ) )
    {
        CLogger::AuthPrintf ( "HTTP: Connection flood from '%s'. Ignoring for 1 min.\n", szAddress );
        return false;
    }

    return true;
}
