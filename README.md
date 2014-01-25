pivotal
=======

Basic plan:
-Inject DLL in target IE process using Metasploit's [Reflective DLL Injection] (http://blog.harmonysecurity.com/2008/10/new-paper-reflective-dll-injection.html)
-DLL's DllMain launches a thread
-The thread starts an HTTP proxy server
-Proxy server listens for HTTP(S) requests
-Proxy server completes responses using WinINet thereby inheriting any associated credentials from the parent process
-Proxy server responds with WinINet response
-Server closes when IE process ends

Current status:
-DllMain launches a thread
-Thread makes a request to a predetermined URL
-Thread writes response to a file
-?
-Profit

To be done:
-Listening for HTTP(S) requests
-Performing arbitrary HTTP(S) requests
-Relaying request response
