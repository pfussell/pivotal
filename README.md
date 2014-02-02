pivotal
=======

Big idea
- Inject DLL in target IE process using Metasploit's [Reflective DLL Injection](http://blog.harmonysecurity.com/2008/10/new-paper-reflective-dll-injection.html)
- DLL's DllMain launches a thread
- The thread starts an HTTP proxy server
- Proxy server listens for HTTP requests
- HTTP CONNECT requests to port 443 are intercepted, the proxy returning "200 Connection established" then initiating handshake as the requested server
- If request is encrypted, it is decrypted using keys established during handshake.
- Proxy server forwards request using WinINet API thereby inheriting any associated credentials from the parent process
- Proxy server forwards response back to original client, reencrypting response if needed.
- Server closes when IE process ends

Current status
- driver.exe loads DLL using LoadLibrary
- DLL's DllMain launches a thread which starts the proxy server
- Server listens for connections on 0.0.0.0:4040
- Incoming requests are parsed, transmitted through WinINet, and returned
- TLS handshakes are partially functional, but won't yet be responded to

Building
- VS2012 solution provided, should work in 2013 too
- Configured for 32- and 64-bit DLLs (which one do we need?)
- Requires some C++11 features though this requirement might be removed in the future. If building in VS2012 which lacks some C++11 features, install (updated compiler)[http://www.microsoft.com/en-us/download/details.aspx?id=35515]

Testing
- Run driver.exe. It expects a dll named pivotal.dll in the same directory.
- After five seconds, server will start on a separate thread
- Set proxy setting to use 0.0.0.0:4040
- All connections will be displayed in the console
- All non-SLL connections should work!
  - Doesn't play nice with Nmap + proxychains, but lower level tools seem to work (need to PCAP this to see whats happening)
  - Netcat will connect but the proxy gives a status of - connection forefully closed by remote host (again need to PCAP this to see why)
  - The solution to both of these may be to A) Fix SSL B) Build a port scanner that will work over/with HTTP
  - Most NON-HTTP requests give me an error of "doens't support SSL yet"

To be done:
- Create a port scanner that will run over HTTP to play nice with our proxy
  - Need to do some testing with making requests to ports over HTTP to see how this will work
- Add HTTPS support. [See example here](http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/example/ssl/server.cpp)
- Actually try injecting this into IE and see what happens
  - Make the dll reflective for injection (https://github.com/stephenfewer/ReflectiveDLLInjection)
- Set up to be delivered with MSF
  - patch the reflective DLL to make it compatible with the dllinject stager
  - deliver the patched reflective DLL to the dllinject stager
- SEE: http://blog.strategiccyber.com/2012/09/17/delivering-custom-payloads-with-metasploit-using-dll-injection/

Steps to Creating a Payload
- Target Vulnerability
- Setting Up for Development
- Choosing a Starting Point
- Development Process Overview
- Triggering the Vulnerability
- Sending the Payload
