#include <thread>

#include "Daemon.h"

#include "Log.h"
#include "base64.h"
#include "Transports.h"
#include "NTCPSession.h"
#include "RouterInfo.h"
#include "RouterContext.h"
#include "Tunnel.h"
#include "NetDb.h"
#include "Garlic.h"
#include "util.h"
#include "Streaming.h"
#include "HTTPServer.h"
#include "HTTPProxy.h"
#include "SOCKS.h"

namespace i2p
{
	namespace util
	{
		class Daemon_Singleton::Daemon_Singleton_Private
		{
		public:
			Daemon_Singleton_Private() : httpServer(nullptr), httpProxy(nullptr), socksProxy(nullptr) { };
			~Daemon_Singleton_Private() {
				delete httpServer;
				delete httpProxy;
				delete socksProxy;
			};

			i2p::util::HTTPServer *httpServer;
			i2p::proxy::HTTPProxy *httpProxy;
			i2p::proxy::SOCKSProxy *socksProxy;
		};

		Daemon_Singleton::Daemon_Singleton() : running(1), d(*new Daemon_Singleton_Private()) {};
		Daemon_Singleton::~Daemon_Singleton() {
			delete &d;
		};


		bool Daemon_Singleton::init(int argc, char* argv[])
		{
			i2p::util::config::OptionParser(argc, argv);

			LogPrint("\n\n\n\ni2pd starting\n");
			LogPrint("data directory: ", i2p::util::filesystem::GetDataDir().string());
			i2p::util::filesystem::ReadConfigFile(i2p::util::config::mapArgs, i2p::util::config::mapMultiArgs);

			isDaemon = i2p::util::config::GetArg("-daemon", 0);
			isLogging = i2p::util::config::GetArg("-log", 1);

			//TODO: This is an ugly workaround. fix it.
			//TODO: Autodetect public IP.
			i2p::context.OverrideNTCPAddress(i2p::util::config::GetCharArg("-host", "127.0.0.1"),
				i2p::util::config::GetArg("-port", 17007));

			LogPrint("CMD parameters:");
			for (int i = 0; i < argc; ++i)
				LogPrint(i, "  ", argv[i]);

			return true;
		}
			
		bool Daemon_Singleton::start()
		{
			// initialize log			
			if (isLogging)
			{
				if (isDaemon)
				{
					std::string logfile_path = i2p::util::filesystem::GetDataDir().string();
	#ifndef _WIN32
					logfile_path.append("/debug.log");
	#else
					logfile_path.append("\\debug.log");
	#endif
					StartLog (logfile_path);
				}
				else
					StartLog (""); // write to stdout
			}

			d.httpServer = new i2p::util::HTTPServer(i2p::util::config::GetArg("-httpport", 7070));
			d.httpServer->Start();
			LogPrint("HTTP Server started");

			i2p::data::netdb.Start();
			LogPrint("NetDB started");
			i2p::transports.Start();
			LogPrint("Transports started");
			i2p::tunnel::tunnels.Start();
			LogPrint("Tunnels started");
			i2p::garlic::routing.Start();
			LogPrint("Routing started");
			i2p::stream::StartStreaming();
			LogPrint("Streaming started");

			d.httpProxy = new i2p::proxy::HTTPProxy(i2p::util::config::GetArg("-httpproxyport", 4446));
			d.httpProxy->Start();
			LogPrint("HTTP Proxy started");
			d.socksProxy = new i2p::proxy::SOCKSProxy(i2p::util::config::GetArg("-socksproxyport", 4447));
			d.socksProxy->Start();
			LogPrint("SOCKS Proxy Started");
			return true;
		}

		bool Daemon_Singleton::stop()
		{
			LogPrint("Shutdown started.");

			d.httpProxy->Stop();
			LogPrint("HTTP Proxy stoped");
			d.socksProxy->Stop();
			LogPrint("SOCKS Proxy stoped");
			i2p::stream::StopStreaming();
			LogPrint("Streaming stoped");
			i2p::garlic::routing.Stop();
			LogPrint("Routing stoped");
			i2p::tunnel::tunnels.Stop();
			LogPrint("Tunnels stoped");
			i2p::transports.Stop();
			LogPrint("Transports stoped");
			i2p::data::netdb.Stop();
			LogPrint("NetDB stoped");
			d.httpServer->Stop();
			LogPrint("HTTP Server stoped");
			StopLog ();
                        delete d.socksProxy; d.socksProxy = nullptr;
			delete d.httpProxy; d.httpProxy = nullptr;
			delete d.httpServer; d.httpServer = nullptr;

			return true;
		}
	}
}
