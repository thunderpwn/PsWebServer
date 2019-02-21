// Copyright 2015-2019 Mail.Ru Group. All Rights Reserved.

#include "PsWebServerWrapper.h"

#include "PsWebServer.h"
#include "PsWebServerDefines.h"
#include "PsWebServerHandler.h"
#include "PsWebServerSettings.h"

#if WITH_CIVET
#include "CivetServer.h"

// CivetWeb Example
class ExampleHandler : public CivetHandler
{
public:
	bool handleGet(CivetServer* server, struct mg_connection* conn)
	{
		mg_printf(conn,
			"HTTP/1.1 200 OK\r\nContent-Type: "
			"text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><body>\r\n");
		mg_printf(conn,
			"<h2>This is an example text from a C++ handler</h2>\r\n");
		mg_printf(conn, "</body></html>\r\n");

		return true;
	}
};
// CivetWeb Example END
#endif // WITH_CIVET

UPsWebServerWrapper::UPsWebServerWrapper(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_CIVET
	Server = nullptr;
#endif
}

void UPsWebServerWrapper::BeginDestroy()
{
	StopServer();

	Super::BeginDestroy();
}

void UPsWebServerWrapper::StartExampleServer()
{
	StartServer();

#if WITH_CIVET
	// Add static example handler
	static ExampleHandler h_ex;
	Server->addHandler("/example", h_ex);
#endif
}

void UPsWebServerWrapper::StartServer()
{
#if WITH_CIVET
	const UPsWebServerSettings* ServerSettings = FPsWebServerModule::Get().GetSettings();

	FString ServerURL = ServerSettings->ServerAddress;
	if (ServerSettings->ServerPort != 0)
	{
		ServerURL.Append(TEXT(":") + FString::FromInt(ServerSettings->ServerPort));
	}

	std::vector<std::string> cpp_options;
	cpp_options.push_back("listening_ports");
	cpp_options.push_back(TCHAR_TO_ANSI(*ServerURL));
	cpp_options.push_back("num_threads");
	cpp_options.push_back(TCHAR_TO_ANSI(*FString::FromInt(ServerSettings->NumTreads)));

	if (ServerSettings->bEnableKeepAlive)
	{
		cpp_options.push_back("enable_keep_alive");
		cpp_options.push_back("yes");
		cpp_options.push_back("keep_alive_timeout_ms");
		cpp_options.push_back(TCHAR_TO_ANSI(*FString::FromInt(ServerSettings->KeepAliveTimeout)));
	}
	else
	{
		cpp_options.push_back("enable_keep_alive");
		cpp_options.push_back("no");
		cpp_options.push_back("keep_alive_timeout_ms");
		cpp_options.push_back("0");
	}

	// Stop server first if one already exists
	if (Server)
	{
		UE_LOG(LogPwsAll, Warning, TEXT("%s: Existing CivetWeb server instance found, stop it now"), *PS_FUNC_LINE);
		StopServer();
	}

	// Run new civet instance
	Server = MakeUnique<CivetServer>(cpp_options);

	// Check that we're really running now
	if (Server->getContext() == nullptr)
	{
		UE_LOG(LogPwsAll, Fatal, TEXT("%s: Failed to run CivetWeb server instance"), *PS_FUNC_LINE);
	}

	UE_LOG(LogPwsAll, Log, TEXT("%s: CivetWeb server instance started on: %s"), *PS_FUNC_LINE, *ServerURL);
#else
	UE_LOG(LogPwsAll, Error, TEXT("%s: Civet is not supported"), *PS_FUNC_LINE);
#endif // WITH_CIVET
}

void UPsWebServerWrapper::StopServer()
{
#if WITH_CIVET
	if (Server)
	{
		Server->close();
		Server = nullptr;

		UE_LOG(LogPwsAll, Log, TEXT("%s: CivetWeb server instance stopped"), *PS_FUNC_LINE);
	}
#endif // WITH_CIVET
}

bool UPsWebServerWrapper::AddHandler(UPsWebServerHandler* Handler, const FString& URI)
{
#if WITH_CIVET
	// Remove previous handler if we had one
	RemoveHandler(URI);

	if (Handler->BindHandler(this, URI))
	{
		BinnedHandlers.Emplace(URI, Handler);

		UE_LOG(LogPwsAll, Log, TEXT("%s: Handler successfully added for URI: %s"), *PS_FUNC_LINE, *URI);
		return true;
	}
#endif // WITH_CIVET

	return false;
}

bool UPsWebServerWrapper::RemoveHandler(const FString& URI)
{
#if WITH_CIVET
	if (Server)
	{
		if (BinnedHandlers.Contains(URI))
		{
			BinnedHandlers.Remove(URI);
		}

		// Remove handler from civet anyway
		Server->removeHandler(TCHAR_TO_ANSI(*URI));

		return true;
	}
#endif // WITH_CIVET

	return false;
}
