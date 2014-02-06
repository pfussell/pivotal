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
- Testing in a lab enviornment verified that the proxy allows access to hosts to which there is an open session.
  - Testing scinaro used:
    - Target host is on a remote subnet that is segmented from the attacker by ACL's and stateful inspection
    - An intermediary can access the target host but only with web traffic
     - The ideal for testing would be if access to the remote host was restriced by an additional itermediary like a jump box because it is possible to mimic HTTP traffic and fool packet inspection
    - We compromise the itermediary execute our payload 
    - We can now interact with any host the user has a session open to

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
