using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using HighwayPursuitServer.Injected;

namespace HighwayPursuitServer
{
    public class InjectionEntryPoint : EasyHook.IEntryPoint
    {
        readonly ServerInterface _server = null;
        readonly Queue<string> _messageQueue = new Queue<string>();

        public InjectionEntryPoint(EasyHook.RemoteHooking.IContext context, string channelName)
        {
            _server = EasyHook.RemoteHooking.IpcConnectClient<ServerInterface>(channelName);
            _server.Ping();
        }

        public void Run(EasyHook.RemoteHooking.IContext context, string channelName)
        {
            // Injection is now complete and the server interface is connected
            _server.IsInstalled(EasyHook.RemoteHooking.GetCurrentProcessId());
            _server.ReportMessage("Hooks installed");

            // Init hook
            void report(string message)
            {
                try
                {
                    lock (this._messageQueue)
                    {
                        if (this._messageQueue.Count < 1000)
                        {
                            var process = EasyHook.RemoteHooking.GetCurrentProcessId();
                            var threadId = EasyHook.RemoteHooking.GetCurrentThreadId();
                            // Add message to send to FileMonitor
                            this._messageQueue.Enqueue(
                                $"[{process}:{threadId}]: {message}");
                        }
                    }
                }
                catch
                {
                    // swallow exceptions so that any issues caused by this code do not crash target process
                }
            }
            var manager = new GameManager(report);

            // Wake up the process (required if using RemoteHooking.CreateAndInject)
            EasyHook.RemoteHooking.WakeUpProcess();

            try
            {
                // Loop until FileMonitor closes (i.e. IPC fails)
                while (true)
                {
                    System.Threading.Thread.Sleep(500);

                    string[] queued = null;

                    lock (_messageQueue)
                    {
                        queued = _messageQueue.ToArray();
                        _messageQueue.Clear();
                    }

                    // Send newly monitored file accesses to FileMonitor
                    if (queued != null && queued.Length > 0)
                    {
                        _server.ReportMessages(queued);
                    }
                    else
                    {
                        _server.Ping();
                    }
                }
            }
            catch
            {
                // Ping() or ReportMessages() will raise an exception if host is unreachable
            }
        }
    }
}
