pivotal
=======

Big idea
- Inject DLL in target IE process using Metasploit's [Reflective DLL Injection](http://blog.harmonysecurity.com/2008/10/new-paper-reflective-dll-injection.html)
- DLL's DllMain launches a thread
- The thread starts an HTTP proxy server
- Proxy server listens for HTTP(S) requests
- Proxy server completes responses using WinINet thereby inheriting any associated credentials from the parent process
- Proxy server responds with WinINet response
- Server closes when IE process ends

Current status
- driver.exe loads DLL using LoadLibrary
- DLL's DllMain launches a thread which starts the proxy server
- Server listens for connections on 127.0.0.1:4040
- Incoming requests are parsed, transmitted through WinINet
- HTTPS requests are trashed for now
- Responses are returned using boost asio
- ?
- Profit

Building
- VS2012 solution provided, should work in 2013 too
- Configured for 64-bit builds only, will be corrected soon
- Requires some C++11 features though this requirement might be removed in the future. If building in VS2012 which lacks C++11 features, install (updated compiler)[http://www.microsoft.com/en-us/download/details.aspx?id=35515]

Testing
- Run driver.exe. It expects a dll named pivotal.dll in the same directory.
- After five seconds, server will start on a separate thread
- Set proxy setting to use 127.0.0.1:4040
- All connections will be displayed in the console
- All non-SLL connections should work!

To be done:
- Add HTTPS support. [See example here](http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/example/ssl/server.cpp)
- Actually try injecting this into IE and see what happens

- How hard will it be to have this process initiate connections to protcols/ports outside of HTTP? 
 - ie. Can the proxy act as a pivot point for any network traffic?

Steps to Creating a Payload
- Target Vulnerability
- Setting Up for Development
- Choosing a Starting Point
- Development Process Overview
- Triggering the Vulnerability
- Sending the Payload
