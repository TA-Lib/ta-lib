/* Note: This file has been adapted for running within TA-LIB.
 * If you intend to use iMatix, please use their original file.
 */
 
/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflsock.c
    Title:      TCP/IP, UDP/IP socket functions
    Package:    Standard Function Library (SFL)

    Written:    1996/02/03  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/19

    Notes:      The BeOS port of sflsock is incomplete: at last testing,
                BeOS 4.5 gave consistent errors on its select() function,
                apparently due to timer interrupt signals.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfllist.h"                    /*  List-management functions        */
#include "sflmem.h"                     /*  Memory-allocation functions      */
#include "sflsymb.h"                    /*  Symbol-table functions           */
#include "sfltok.h"                     /*  Token-handling functions         */
#include "sfluid.h"                     /*  User/group functions             */
#include "sflcons.h"                    /*  Console i/o functions            */
#include "sflfile.h"                    /*  File handling functions          */
#include "sflprint.h"                   /*  snprintf functions               */
#include "sflsock.h"                    /*  Prototypes for functions         */

#include "ta_memory.h"
#include "ta_trace.h"
TA_FILE_INFO;

/*  Implementation notes
 *
 *  These functions work on 16-bit Windows, 32-bit Windows, 32-bit UNIX,
 *  64-bit UNIX, Digital OpenVMS.  The size of a socket handle varies from
 *  16 bits to 64 bits.  All native socket functions define a socket handle
 *  as 'int'.  However, we need a fixed-length external representation.  So,
 *  we define a type, 'sock_t', which is a qbyte (32 bits).  Outside this
 *  package, sockets are always a 'sock_t'.  Internally, we always use an
 *  (SOCKET) cast when passing s sock_t to a system function like connect().
 *  If the system does not support sockets we fake them just a little.
 *
 *  Modifications Oct 7 1998 for Unix by Grant McDorman <grant@isgtec.com> to
 *  allow running with the program suid root; it will run as the user until
 *  the socket must be opened; at that time, it will briefly switch to root
 *  and then return to the actual user id.
 */

/*  Global variables                                                         */

int
    ip_portbase = 0;                    /*  Base for created services        */
Bool
    ip_nonblock = TRUE;                 /*  Create non-blocking sockets      */
qbyte
    ip_passive = INADDR_ANY;            /*  IP address for passive connects  */
int
    ip_sockets = 0;                     /*  Number of open sockets           */


/*  The connect_error_value holds the last recorded error cause after a      */
/*  connection attempt.                                                      */

static int
    connect_error_value = 0;
char
    *connect_errlist [] = {             /*  Corresponding error messages     */
        "No errors",
        "System does not support sockets",
        "Host is not known",
        "Service or port not known",
        "Protocol not known",
        "Connection failed on socket()",
        "Connection failed on connect()",
        "Port is already used by another server",
        "Connection failed on listen()"
    };

/*  Internal functions used to create passive and active connections         */

#if (defined (DOES_SOCKETS))
static void   prepare_socket (TA_Libc *libHandle, sock_t handle);
#   if (defined (__WINDOWS__))
static int    win_error      (TA_Libc *libHandle, int rc);
#   endif
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: sock_init

    Synopsis: Initialise the internet protocol.  On most systems this is a
    null call.  On some systems this loads dynamic libraries.  Returns 0
    0 if everything was okay, else returns SOCKET_ERROR.  You should call
    sock_term() when your program ends.
    ---------------------------------------------------------------------[>]-*/

int
sock_init (TA_Libc *libHandle)
{

#if (defined (__WINDOWS__))
    WORD
        wVersionRequested;              /*  We really want Winsock 1.1       */
    WSADATA
        wsaData;

   (void )libHandle; /* Get ride of compiler warnings. */

    wVersionRequested = 0x0101;         /*  ... but we'll take 1.1           */
    if (WSAStartup (wVersionRequested, &wsaData) == 0)
        return (0);
    else
        return ((int) SOCKET_ERROR);

#elif (defined (__UTYPE_BEOS))
    /*  BeOS numbers sockets from 0 upwards, but this causes havoc with
     *  programs that expect a BSD-style numbering of 1 or higher.  We
     *  force compatibility by creating (and wasting) one socket so that
     *  further socket handles are guaranteed >0.
     */
    create_socket ("tcp");
    return (0);

#elif (defined (DOES_SOCKETS))
    return (0);

#elif (defined (FAKE_SOCKETS))
    return (0);

#else
    connect_error_value = IP_NOSOCKETS;
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: sock_term

    Synopsis: Shuts-down the internet protocol.  On most systems this is a
    null call.  On some systems this unloads dynamic libraries.  Returns -1
    if there was an error, or 0 if everything was okay.  See sock_init().
    ---------------------------------------------------------------------[>]-*/

int
sock_term (TA_Libc *libHandle)
{
   (void )libHandle; /* Get ride of compiler warnings. */

#if (defined (__WINDOWS__))
    WSACleanup ();
#endif
    return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: passive_TCP

    Synopsis: Creates a passive bound TCP socket for the specified service.
    Returns socket number or INVALID_SOCKET.  If it returns INVALID_SOCKET,
    you can get the reason for the error by calling connect_error ().  This
    may be one of:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADSERVICE       Service cannot be converted to port number
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot create the passive socket
    IP_BINDERROR        Cannot bind to the port
    IP_LISTENERROR      Cannot listen to port
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

sock_t
passive_TCP (
    TA_Libc *libHandle,
    const char *service,                /*  Service name or port as string   */
    int queue_length                    /*  Queue length for listen()        */
)
{    
    TA_ASSERT_RET(libHandle, service && *service, INVALID_SOCKET );
    TA_ASSERT_RET(libHandle, queue_length > 0, INVALID_SOCKET );
    return (passive_socket (libHandle,service, "tcp", queue_length));
}


/*  ---------------------------------------------------------------------[<]-
    Function: passive_UDP

    Synopsis: Creates a passive UDP socket for the specified service.
    Returns socket number or INVALID_SOCKET.  If it returns INVALID_SOCKET,
    you can get the reason for the error by calling connect_error ().  This
    may be one of:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADSERVICE       Service cannot be converted to port number
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot create the passive socket
    IP_BINDERROR        Cannot bind to the port
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

sock_t
passive_UDP (
    TA_Libc *libHandle,
    const char *service                 /*  Service name or port as string   */
)
{
    TA_ASSERT_RET(libHandle, service && *service, INVALID_SOCKET);
    return (passive_socket (libHandle,service, "udp", 0));
}


/*  ---------------------------------------------------------------------[<]-
    Function: passive_socket

    Synopsis:
    Creates a passive TCP or UDP socket.  This function allows a server
    program to create a master socket, so that connections can be accepted.
    Used by the passive_TCP and passive_UDP functions.  Returns a socket
    number or INVALID_SOCKET.  If it returns INVALID_SOCKET, you can get the
    reason for the error by calling connect_error ().  This may be one of:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADSERVICE       Service cannot be converted to port number
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot create the passive socket
    IP_BINDERROR        Cannot bind to the port
    IP_LISTENERROR      Cannot listen to port
    </TABLE>
    By default, opens a socket on all available IP addresses.  You can open
    the socket on a specific address, by setting the global variable
    ip_passive to the address (in network order).  This variable is reset
    to INADDR_ANY after each call to passive_socket or one of the functions
    that calls it.
    ---------------------------------------------------------------------[>]-*/

sock_t
passive_socket (
    TA_Libc *libHandle,
    const char *service,                /*  Service name or port as string   */
    const char *protocol,               /*  Protocol "tcp" or "udp"          */
    int queue_length                    /*  Queue length for TCP sockets     */
)
{
#if (defined (DOES_SOCKETS))
    struct servent
        *pse;                           /*  Service information entry        */
    struct sockaddr_in
        sin;                            /*  Internet end-point address       */
    sock_t
        handle;                         /*  Socket from socket() call        */

    TA_ASSERT_RET(libHandle, service && *service, INVALID_SOCKET );
    TA_ASSERT_RET(libHandle, protocol && *protocol, INVALID_SOCKET );

    connect_error_value = IP_NOERROR;   /*  Assume no errors                 */

    memset ((void *) &sin, 0, sizeof (sin));
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = ip_passive;
    ip_passive = INADDR_ANY;            /*  Reset passive address            */

    /*  To allow privileged operations, if possible                          */
    set_uid_root ();

    /*  Map service name to port number                                      */
    pse = getservbyname (service, protocol);
    if (pse)
        sin.sin_port = htons ((dbyte) (ntohs (pse-> s_port) + ip_portbase));
    else
      {
        sin.sin_port = (unsigned short)atoi (service);
        if (sin.sin_port + ip_portbase > 0)
            sin.sin_port = htons ((dbyte) (sin.sin_port + ip_portbase));
        else
          {
            connect_error_value = IP_BADSERVICE;
            set_uid_user ();
            return (INVALID_SOCKET);
          }
      }
    handle = create_socket (libHandle,protocol);
    if (handle == INVALID_SOCKET)       /*  Cannot create the socket         */
      {
        set_uid_user ();
        return (INVALID_SOCKET);
      }

    /*  Bind the socket                                                      */
    if (bind ((SOCKET) handle, (struct sockaddr *) &sin,
        sizeof (sin)) == SOCKET_ERROR)
      {
        connect_error_value = IP_BINDERROR;
        set_uid_user ();
        return (INVALID_SOCKET);        /*  Cannot bind to port              */
      }
    set_uid_user ();

    /*  Specify incoming queue length for stream socket                      */
    if (streq (protocol, "tcp")
    && listen ((SOCKET) handle, queue_length) == SOCKET_ERROR)
      {
        connect_error_value = IP_LISTENERROR;
        return (INVALID_SOCKET);        /*  Cannot listen on port            */
      }
    return (handle);

#elif (defined (FAKE_SOCKETS))
    return (1);                         /*  Return dummy handle              */

#else
    connect_error_value = IP_NOSOCKETS;
    return (INVALID_SOCKET);            /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: create_socket

    Synopsis:
    Creates a TCP or UDP socket.  The socket is not connected.  To use
    with TCP services you must bind or connect the socket.  You can use
    the socket with UDP services - e.g. read_UDP () - immediately.  Returns
    a socket number or INVALID_SOCKET, in which case you can get the reason
    for the error by calling connect_error ().  This may be one of:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot create the socket
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

sock_t
create_socket (
    TA_Libc *libHandle,
    const char *protocol                /*  Protocol "tcp" or "udp"          */
)
{
#if (defined (DOES_SOCKETS))
    struct protoent
        *ppe;                           /*  Protocol information entry       */
    int
#   if (!defined (__WINDOWS__))
        true_value = 1,                 /*  Boolean value for setsockopt()   */
#   endif
        sock_type;                      /*  Type of socket we want           */
    sock_t
        handle;                         /*  Socket from socket() call        */

    TA_ASSERT_RET (libHandle,protocol && *protocol, INVALID_SOCKET );
    connect_error_value = IP_NOERROR;   /*  Assume no errors                 */

    /*  Map protocol name to protocol number                                 */
    ppe = getprotobyname (protocol);
    if (ppe == NULL)                    /*  Cannot get protocol entry        */
      {
        connect_error_value = IP_BADPROTOCOL;
        return (INVALID_SOCKET);
      }
    /*  Use protocol string to choose a socket type                          */
    if (streq (protocol, "udp"))
        sock_type = SOCK_DGRAM;
    else
        sock_type = SOCK_STREAM;

    /*  Allocate a socket                                                    */
    handle = (sock_t) socket (AF_INET, sock_type, ppe-> p_proto);
    if (handle == INVALID_SOCKET)       /*  Cannot create passive socket     */
      {
        connect_error_value = IP_SOCKETERROR;
        return (INVALID_SOCKET);
      }
#   if (!defined (__WINDOWS__))
    /*  On BSD-socket systems we need to do this to allow the server to
     *  restart on a previously-used socket, without an annoying timeout
     *  of several minutes.  With winsock the reuseaddr option lets the
     *  server work with an already-used socket (!), so we don't do it.
     */
    setsockopt ((SOCKET) handle, SOL_SOCKET, SO_REUSEADDR,
                (char *) &true_value, sizeof (true_value));
#   endif
    prepare_socket (libHandle, handle);            /*  Ready socket for use             */
    ip_sockets++;
    return (handle);

#elif (defined (FAKE_SOCKETS))
    return (1);                         /*  Return dummy handle              */

#else
    connect_error_value = IP_NOSOCKETS;
    return (INVALID_SOCKET);            /*  Sockets not supported            */
#endif
}


#if (defined (DOES_SOCKETS))
/*  -------------------------------------------------------------------------
 *  prepare_socket -- internal
 *
 *  Does any system-specific work required to prepare a socket for normal
 *  use.  In Windows we have to set the socket to nonblocking mode.  In
 *  UNIX we do this if the ip_nonblock flag is set.
 */

static void
prepare_socket (TA_Libc *libHandle, sock_t handle)
{
#if (defined (__WINDOWS__))
    u_long
        command = ip_nonblock? 1: 0;

   (void )libHandle; /* Get ride of compiler warnings. */

    /*  Redirect events and set non-blocking mode                            */
    if (handle != INVALID_SOCKET)
        ioctlsocket ((SOCKET) handle, FIONBIO, &command);

#elif (defined (__UTYPE_BEOS))
    setsockopt ((SOCKET) handle, SOL_SOCKET, SO_NONBLOCK,
                (void *) &ip_nonblock, sizeof (ip_nonblock));

#elif (defined (__UNIX__) || defined (__OS2__))
    if (ip_nonblock)
        fcntl ((SOCKET) handle, F_SETFL, O_NONBLOCK
                | fcntl (handle, F_GETFL, 0));
#endif
}
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: connect_TCP

    Synopsis:
    Creates a TCP socket and connects it to a specified host and service.
    Returns a socket number or INVALID_SOCKET.  In that case you can get
    the reason for the error by calling connect_error ().  This may be:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADHOST          Host is not known
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot open a socket
    IP_CONNECTERROR     Cannot connect socket
    </TABLE>
    The host name may be a full name, NULL or "" meaning the current host,
    or a dotted-decimal number.  The service may be a defined service, e.g.
    "echo", or a port number, specified as an ASCII string.  See
    connect_socket() for details.

    Single-threaded clients may set ip_nonblock to FALSE and block on all
    read and write operations.   They may use select() if they need to be
    able to time-out during reading/writing.

    Multi-threaded servers should set ip_nonblock to TRUE, and use select()
    to multiplex socket access.  When ip_nonblock is TRUE, connect calls
    will return immediately, and the server should use select() to wait until
    the socket is ready for writing.  On some systems (early Linux?), the
    select() call will fail in this situation.  If you compile with
    -DBLOCKING_CONNECT, connects are done synchronously in all cases.

    Examples:
    sock_t handle;
    handle = connect_TCP ("", "8080");
    handle = connect_TCP (NULL, "echo");
    handle = connect_TCP ("www.imatix.com", "http");
    ---------------------------------------------------------------------[>]-*/

sock_t
connect_TCP (
    TA_Libc *libHandle,
    const char *host,                   /*  Host name                        */
    const char *service                 /*  Service name                     */
)
{
    TA_ASSERT_RET (libHandle,service && *service, INVALID_SOCKET );
    return (connect_socket (libHandle,
                            host,       /*  We have a host name              */
                            service,    /*  We have a service name           */
                            "tcp",      /*  Protocol is TCP                  */
                            NULL,       /*  No prepared address              */
                            3, 0));     /*  3 retries, no waiting            */
}


/*  ---------------------------------------------------------------------[<]-
    Function: connect_UDP

    Synopsis:
    Creates a UDP socket and connects it to a specified host and service.
    Returns a socket number or INVALID_SOCKET.  In that case you can get
    the reason for the error by calling connect_error ().  This may be:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADHOST          Host is not known
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot open a socket
    IP_CONNECTERROR     Cannot connect socket
    </TABLE>
    The host name may be a full name, NULL or "" meaning the current host,
    or a dotted-decimal number.  The service may be a defined service, e.g.
    "echo", or a port number, specified as an ASCII string.  See
    connect_socket() for details.

    Single-threaded clients may set ip_nonblock to FALSE and block on all
    read and write operations.   They may use select() if they need to be
    able to time-out during reading/writing.

    Multi-threaded servers should set ip_nonblock to TRUE, and use select()
    to multiplex socket access.  When ip_nonblock is TRUE, connect calls
    will return immediately, and the server should use select() to wait until
    the socket is ready for writing.  On some systems (early Linux?), the
    select() call will fail in this situation.  If you compile with
    -DBLOCKING_CONNECT, connects are done synchronously in all cases.

    Examples:
    sock_t handle;
    handle = connect_UDP ("", "7");
    handle = connect_UDP (NULL, "echo");
    handle = connect_UDP ("imatix.com", "echo");
    ---------------------------------------------------------------------[>]-*/

sock_t
connect_UDP (
    TA_Libc *libHandle,
    const char *host,                   /*  Host name                        */
    const char *service                 /*  Service name                     */
)
{
    TA_ASSERT_RET (libHandle,service && *service, INVALID_SOCKET );
    return (connect_socket (libHandle,
                            host,       /*  We have a host name              */
                            service,    /*  We have a service name           */
                            "udp",      /*  Protocol is UDP                  */
                            NULL,       /*  No prepared address              */
                            3, 0));     /*  3 retries, no waiting            */
}


/*  ---------------------------------------------------------------------[<]-
    Function: connect_TCP_fast

    Synopsis: Creates a TCP socket and connects it to a specified host/port
    address.  Returns a socket number or INVALID_SOCKET.  In that case you
    can get the reason for the error by calling connect_error ().  This may
    be:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADHOST          Host is not known
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot open a socket
    IP_CONNECTERROR     Cannot connect socket
    </TABLE>
    This function is faster, if you know the host system address and port,
    than connect_TCP() because no translation is needed.
    You can get the host/address structure by calling address_end_point()
    or get_peer_addr().  See connect_socket() for details.
    ---------------------------------------------------------------------[>]-*/

sock_t
connect_TCP_fast (
    TA_Libc *libHandle,
    const struct sockaddr_in *sin       /*  Socket address structure         */
)
{
    TA_ASSERT_RET (libHandle,sin,INVALID_SOCKET);
    return (connect_socket (libHandle,
                            NULL,       /*  No host name                     */
                            NULL,       /*  No service name                  */
                            "tcp",      /*  Protocol is TCP                  */
                            sin,        /*  We have a prepared address       */
                            1, 0));     /*  1 retry, no waiting              */
}


/*  ---------------------------------------------------------------------[<]-
    Function: connect_UDP_fast

    Synopsis:
    Creates a UDP socket and connects it to a specified host/port address.
    Returns a socket number or INVALID_SOCKET.  In that case you can get
    the reason for the error by calling connect_error ().  This may be:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADHOST          Host is not known
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot open a socket
    IP_CONNECTERROR     Cannot connect socket
    </TABLE>
    This function is faster, if you know the host system address and port,
    than connect_UDP() because no translation is needed.
    You can get the host/address structure by calling address_end_point()
    or get_peer_addr().  See connect_socket() for details.
    ---------------------------------------------------------------------[>]-*/

sock_t
connect_UDP_fast (
    TA_Libc *libHandle,
    const struct sockaddr_in *sin       /*  Socket address structure         */
)
{
    TA_ASSERT_RET (libHandle,sin,INVALID_SOCKET);
    return (connect_socket (libHandle,
                            NULL,       /*  No host name                     */
                            NULL,       /*  No service name                  */
                            "udp",      /*  Protocol is UDP                  */
                            sin,        /*  We have a prepared address       */
                            1, 0));     /*  1 retry, no waiting              */
}


/*  ---------------------------------------------------------------------[<]-
    Function: connect_socket

    Synopsis:
    Makes a connection to a remote TCP or UDP port.  This allows a client
    program to start sending information to a server.  Used by the
    connect_TCP and connect_UDP functions.  Returns a socket number or
    INVALID_SOCKET.  If it returns INVALID_SOCKET, you can get the reason
    for the error by calling connect_error ().  This may be one of:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADHOST          Host is not known
    IP_BADPROTOCOL      Cannot understand protocol name
    IP_SOCKETERROR      Cannot open a socket
    IP_CONNECTERROR     Cannot connect socket
    </TABLE>
    Always blocks until the connection has been made; i.e. when this
    function returns you can start to read and write on the socket.

    The host name may be a full name, NULL or "" meaning the current host,
    or a dotted-decimal number.  The service may be a defined service, e.g.
    "echo", or a port number, specified as an ASCII string.  Alternatively,
    both these values may be NULL or "", in which case the function uses
    the host_addr argument to supply an address.  If you want to build the
    host_addr structure yourself, use build_sockaddr().

    Single-threaded clients may set ip_nonblock to FALSE and block on all
    read and write operations.   They may use select() if they need to be
    able to time-out during reading/writing.

    Multi-threaded servers should set ip_nonblock to TRUE, and use select()
    to multiplex socket access.  When ip_nonblock is TRUE, connect calls
    will return immediately, and the server should use select() to wait until
    the socket is ready for writing.  On some systems (early Linux?), the
    select() call will fail in this situation.  If you compile with
    -DBLOCKING_CONNECT, connects are done synchronously in all cases.

    Examples:
    struct sockaddr_in
        host_addr;
    sock_t
        handle;
    build_sockaddr (&host_addr, 32_bit_host, 16_bit_port);
    handle = connect_socket (NULL, NULL, "tcp", &host_addr, 3, 0);
    ---------------------------------------------------------------------[>]-*/

sock_t
connect_socket (
    TA_Libc *libHandle,
    const char *host,                   /*  Name of host, "" = localhost     */
    const char *service,                /*  Service name or port as string   */
    const char *protocol,               /*  Protocol "tcp" or "udp"          */
    const struct sockaddr_in *host_addr, /* Socket address structure         */
    int retries_left,                   /*  Max. number of retries           */
    int retry_delay                     /*  Delay between retries            */
)
{
#if (defined (DOES_SOCKETS))
    struct sockaddr_in
        sin;                            /*  Internet end-point address       */
    sock_t
        handle = 0;                     /*  Created socket                   */
    int
        rc;                             /*  Return code from call            */
    Bool
        old_nonblock;                   /*  Create non-blocking sockets      */

    connect_error_value = IP_NOERROR;   /*  Assume no errors                 */

    /*  Format sockaddr_in port and hostname, and quit if that failed        */
    if (service && strused (service))
      {
        TA_ASSERT_RET (libHandle,protocol && *protocol, INVALID_SOCKET);
        if (address_end_point (libHandle, host, service, protocol, &sin))
            return (INVALID_SOCKET);
      }
    else
      {
        TA_ASSERT_RET (libHandle,host_addr,INVALID_SOCKET);
        sin = *host_addr;               /*  Fast connect requested           */
      }
    /*  Connect socket and maybe retry a few times...                        */
    old_nonblock = ip_nonblock;
#   if (defined (BLOCKING_CONNECT))
    ip_nonblock = FALSE;                /*  Block on this socket             */
#   endif

    while (retries_left)
      {
        handle = create_socket (libHandle, protocol);
        if (handle == INVALID_SOCKET)   /*  Unable to open a socket          */
          {
            ip_nonblock = old_nonblock;
            return (INVALID_SOCKET);
          }
        rc = connect ((SOCKET) handle, (struct sockaddr *) &sin, sizeof (sin));
        if (rc == 0)
            break;                      /*  Connected okay                   */
        else
          {
#           if (defined (__WINDOWS__))
            if (WSAGetLastError () == WSAEWOULDBLOCK)
#           else
            if (errno == EINPROGRESS)
#           endif
                break;                  /*  Still connecting, but okay       */
          }
        /*  Retry if we have any attempts left                               */
        close_socket (libHandle, handle);
        if (--retries_left == 0)      /*  Connection failed                */
          {
            connect_error_value = IP_CONNECTERROR;
            ip_nonblock = old_nonblock;
            return (INVALID_SOCKET);
          }
        sleep (retry_delay);
      }
    ip_nonblock = old_nonblock;
    prepare_socket (libHandle, handle);            /*  Set final blocking mode          */
    return (handle);

#elif (defined (FAKE_SOCKETS))
    return (1);                         /*  Return dummy handle              */

#else
    connect_error_value = IP_NOSOCKETS;
    return (INVALID_SOCKET);            /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: connect_to_peer

    Synopsis:
    Connects an unconnected TCP or UDP socket to a peer specified by a
    sockaddr structure.  Returns 0 if the connection succeeded, or
    SOCKET_ERROR if there was a problem.  In the latter case you can
    get the reason for the error by calling sockmsg().
    ---------------------------------------------------------------------[>]-*/

int
connect_to_peer (
    TA_Libc *libHandle,
    sock_t handle,                      /*  Socket to connect                */
    const struct sockaddr_in *sin       /*  Socket address structure         */
)
{
#if (defined (DOES_SOCKETS))
    int
        rc;                             /*  Return code from call            */
    Bool
        old_nonblock;                   /*  Create non-blocking sockets      */

    TA_ASSERT_RET (libHandle,sin,SOCKET_ERROR);
    old_nonblock = ip_nonblock;
#   if (defined (BLOCKING_CONNECT))
    ip_nonblock = FALSE;                /*  Block on this socket             */
#   endif

    rc = connect ((SOCKET) handle, (struct sockaddr *) sin, sizeof (*sin));

    ip_nonblock = old_nonblock;
    prepare_socket (libHandle, handle);            /*  Set final blocking mode          */

#   if (defined (__WINDOWS__))
    return (win_error (libHandle, rc));
#   else
    return (rc);
#   endif

#else
    connect_error_value = IP_NOSOCKETS;
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: address_end_point

    Synopsis:
    Formats an address block (struct sockaddr_in) for the specified host
    and service (port) information.  Returns 0 if okay, SOCKET_ERROR if
    there was an error, in which case you can call connect_error () to get
    the reason for the error.  This may be one of:
    <TABLE>
    IP_NOSOCKETS        Sockets not supported on this system
    IP_BADHOST          Host is not known
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

int
address_end_point (
    TA_Libc *libHandle,
    const char *host,                   /*  Name of host, "" = localhost     */
    const char *service,                /*  Service name or port as string   */
    const char *protocol,               /*  Protocol "tcp" or "udp"          */
    struct sockaddr_in *sin             /*  Block for formatted address      */
)
{
#if (defined (DOES_SOCKETS))
    struct hostent
        *phe;                           /*  Host information entry           */
    struct servent
        *pse;                           /*  Service information entry        */
    char
        hostname [MAXHOSTNAMELEN + 1];  /*  Name of this system              */
    int
        feedback = 0;                   /*  Assume everything works          */

    TA_ASSERT_RET (libHandle,service && *service,SOCKET_ERROR);
    TA_ASSERT_RET (libHandle,protocol && *protocol,SOCKET_ERROR);
    TA_ASSERT_RET (libHandle,sin,SOCKET_ERROR);

    connect_error_value = IP_NOERROR;   /*  Assume no errors                 */
    memset ((void *) sin, 0, sizeof (*sin));
    sin-> sin_family = AF_INET;

    /*  Map service name to a port number                                    */
    pse = getservbyname (service, protocol);
    if (pse)
        sin-> sin_port = htons ((short) (ntohs (pse-> s_port)));
    else
        sin-> sin_port = htons ((short) (atoi (service)));

    /*  Map host name to IP address, allowing for dotted decimal             */
    if (host && strused (host))
        strcpy (hostname, host);
    else
        strcpy (hostname, "127.0.0.1");

    /*  Check if it's a valid IP address first                               */
    sin-> sin_addr.s_addr = inet_addr (hostname);
    if (sin-> sin_addr.s_addr == INADDR_NONE)
      {
        /*  Not a dotted address -- try to translate the name                */
        phe = gethostbyname (hostname);
        if (phe)
            memcpy ((void *) &sin-> sin_addr, phe-> h_addr, phe-> h_length);
        else
          {                             /*  Cannot map to host               */
            connect_error_value = IP_BADHOST;
            feedback = (int) SOCKET_ERROR;
          }
      }
    return (feedback);

#else
    connect_error_value = IP_NOSOCKETS;
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: build_sockaddr

    Synopsis:
    Builds a socket address structure from the specified host and port
    addresses.  Does not return any value except the built structure.
    ---------------------------------------------------------------------[>]-*/

void
build_sockaddr (
    TA_Libc *libHandle,
    struct sockaddr_in *sin,            /*  Socket address structure         */
    qbyte host,                         /*  32-bit host address              */
    dbyte port                          /*  16-bit port number               */
)
{
    TA_ASSERT_NO_RET (libHandle,sin);

    sin-> sin_family      = AF_INET;
    sin-> sin_addr.s_addr = htonl (host);
    sin-> sin_port        = htons (port);
}


/*  ---------------------------------------------------------------------[<]-
    Function: socket_localaddr

    Synopsis: Returns a string containing the local host address for the
    specified connected socket.  The string is formatted as a string
    "n.n.n.n".  Returns the address of a static string or a buffer that
    is overwritten by each call.  If sockets are not supported, or there
    was an error, returns the loopback address "127.0.0.1".
    ---------------------------------------------------------------------[>]-*/

char *
socket_localaddr (
    TA_Libc *libHandle,
    sock_t handle)
{
#define NTOA_MAX    16
#if (defined (DOES_SOCKETS))
    static char
        localaddr [NTOA_MAX + 1];       /*  xxx.xxx.xxx.xxx                  */
    struct sockaddr_in
        sin;                            /*  Address of local system          */

    if (get_sock_addr (libHandle, handle, &sin, NULL, 0))
        return ("127.0.0.1");
    else
      {
        strncpy (localaddr, inet_ntoa (sin.sin_addr), NTOA_MAX);
        return  (localaddr);
      }
#else
    return ("127.0.0.1");
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: socket_peeraddr

    Synopsis: Returns a string containing the peer host address for the
    specified connected socket.  The string is formatted as a string
    "n.n.n.n".  Returns the address of a static string or a buffer that
    is overwritten by each call.  If sockets are not supported, or there
    was an error, returns the loopback address "127.0.0.1".
    ---------------------------------------------------------------------[>]-*/

char *
socket_peeraddr (
    TA_Libc *libHandle,
    sock_t handle)
{
#if (defined (DOES_SOCKETS))
    static char
        peeraddr [NTOA_MAX + 1];        /*  xxx.xxx.xxx.xxx                  */
    struct sockaddr_in
        sin;                            /*  Address of peer system           */

    if (get_peer_addr (libHandle, handle, &sin, NULL, 0))
        return ("127.0.0.1");
    else
      {
        strncpy (peeraddr, inet_ntoa (sin.sin_addr), NTOA_MAX);
        return  (peeraddr);
      }
#else
    return ("127.0.0.1");
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: socket_nodelay

    Synopsis: Disables Nagle's algorithm for the specified socket; use this
    when you want to ensure that data is sent outwards as fast as possible,
    and when you are certain that Nagle's algorithm is causing a slowdown in
    performance.  Recommended for HTTP, but not recommended for telnet.
    Returns 0 if okay, SOCKET_ERROR if there was a problem.
    ---------------------------------------------------------------------[>]-*/

int
socket_nodelay (
    TA_Libc *libHandle,
    sock_t handle)
{
   int true_value = 1;

   (void)libHandle; /* Get ride of compiler watnings. */
#if (defined (__WINDOWS__))
    return (setsockopt ((SOCKET) handle, IPPROTO_TCP, TCP_NODELAY,
                        (char *) &true_value, sizeof (true_value)));

#elif (defined (TCP_NODELAY) && defined (SOL_TCP))
    return (setsockopt ((SOCKET) handle, SOL_TCP, TCP_NODELAY,
                        (char *) &true_value, sizeof (true_value)));
#else
    return (0);                         /*  Not applicable to this system    */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: socket_is_alive

    Synopsis:
    Returns TRUE if the socket is open.  Returns FALSE if the socket is no
    longer accessible.  You can use this function to check that a socket has
    been closed by the other party, before doing reading or writing.
    ---------------------------------------------------------------------[>]-*/

Bool
socket_is_alive (
    TA_Libc *libHandle,
    sock_t handle)
{   
#if (defined (__UTYPE_BEOS))
    /*  BeOS 4.5 does not support the getsockopt() function                  */
    int
        rc;

   (void)libHandle; /* Get ride of compiler warnings. */

    rc = setsockopt ((SOCKET) handle, SOL_SOCKET, SO_NONBLOCK,
                     (void *) &ip_nonblock, sizeof (ip_nonblock));
    return (rc == 0);

#elif (defined (DOES_SOCKETS))
    int
        socket_type,
        rc;
    argsize_t
        socket_type_size = sizeof (SOCKET);

   (void)libHandle; /* Get ride of compiler warnings. */

    rc = getsockopt ((SOCKET) handle, SOL_SOCKET, SO_TYPE,
                    (char *) &socket_type, &socket_type_size);
    return (unsigned short)(rc == 0);

#else
    (void)libHandle; /* Get ride of compiler warnings. */
    (void)handle;    /* Get ride of compiler warnings. */
    return (FALSE);
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: socket_error

    Synopsis: Returns an errno value for the socket, or 0 if no error was
    outstanding on the socket.  This function is useful if you are handling
    sockets using the select() function: this may return error indicators
    on sockets, without precision on the type of error.  This function will
    return the precise error number.  Errors like EINPROGRESS, EAGAIN, and
    EWOULDBLOCK can usually be ignored or handled by retrying.
    ---------------------------------------------------------------------[>]-*/

int
socket_error (
    TA_Libc *libHandle,
    sock_t handle)
{
    (void)libHandle; /* Get ride of compiler warnings. */
    (void)handle;

    return (errno);

#if 0
#if (defined (DOES_SOCKETS))
#   if (defined (__UTYPE_BEOS))
    return (errno);
#   else
    return (getsockopt ((SOCKET) handle, SOL_SOCKET, SO_ERROR, 0, 0));
#   endif
#else
    return (0);
#endif
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: accept_socket

    Synopsis: Accepts a connection on a specified master socket.  If you
    do not want to wait on this call, use select() to poll the socket until
    there is an incoming request, then call accept_socket.  Returns the
    number of the new slave socket, or INVALID_SOCKET if there was an error
    on the accept call.  You can handle errors as fatal except for EAGAIN
    which indicates that the operation would cause a non-blocking socket to
    block (treat EWOULDBLOCK in the same way).
    ---------------------------------------------------------------------[>]-*/

sock_t
accept_socket (
    TA_Libc *libHandle,
    sock_t master_socket)
{
#if (defined (DOES_SOCKETS))
    sock_t
        slave_socket;                   /*  Connected slave socket           */
    struct sockaddr_in
        sin;                            /*  Address of connecting party      */
    argsize_t
        sin_length;                     /*  Length of address                */

    connect_error_value = IP_NOERROR;   /*  Assume no errors                 */

    sin_length = (int) sizeof (sin);
    slave_socket = accept ((SOCKET) master_socket,
                          (struct sockaddr *) &sin, &sin_length);

    /*  On non-Windows systems, accept returns -1 in case of error, which    */
    /*  is the same as INVALID_SOCKET.                                       */
#   if (defined (__WINDOWS__))
    if (slave_socket == INVALID_SOCKET)
      {
        int sock_errno = WSAGetLastError ();
        if (sock_errno == WSAEWOULDBLOCK || sock_errno == WSAEINPROGRESS)
            errno = EAGAIN;
      }
#   endif
    if (slave_socket != INVALID_SOCKET)
      {
        prepare_socket (libHandle, slave_socket);
        ip_sockets++;
      }
    return (slave_socket);
#else
    connect_error_value = IP_NOSOCKETS;
    return (INVALID_SOCKET);            /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: connect_error

    Synopsis:
    Returns the last error code from one of the connection functions.  For
    portability in a multithreaded environment, call immediately after the
    call to the connection function.
    ---------------------------------------------------------------------[>]-*/

int
connect_error (TA_Libc *libHandle)
{
    (void)libHandle; /* Get ride of compiler warnings. */

    return (connect_error_value);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_sock_addr

    Synopsis: Builds an address block (struct sockaddr_in) for the local
    end of the specified connected socket.  Returns 0 if okay, SOCKET_ERROR
    if there was an error.  If the name argument is not null, looks-up the
    host name and returns it.  The name is truncated to namesize characters,
    including a trailing null character.
    ---------------------------------------------------------------------[>]-*/

int
get_sock_addr (
    TA_Libc *libHandle,
    sock_t handle,                      /*  Socket to get address for        */
    struct sockaddr_in *sin,            /*  Block for formatted address      */
    char *name,                         /*  Buffer for host name, or NULL    */
    int namesize                        /*  Size of host name buffer         */
)
{
#if (defined (DOES_SOCKETS))
    int
        rc;                             /*  Return code from call            */
    struct hostent
        *phe;                           /*  Host information entry           */
    argsize_t
        sin_length;                     /*  Length of address                */

    TA_ASSERT_RET (libHandle,sin,SOCKET_ERROR);

    /*  Get address for local connected socket                               */
    sin_length = sizeof (struct sockaddr_in);

    rc = getsockname ((SOCKET) handle, (struct sockaddr *) sin, &sin_length);

    /*  Translate into host name string, only if wanted                      */
    if (name != NULL && rc == 0)
      {
        phe = gethostbyaddr ((char *) &sin-> sin_addr,
                             sizeof (sin-> sin_addr), AF_INET);
        if (phe)
          {
            strncpy (name, phe-> h_name, namesize);
            name [namesize - 1] = '\0';
          }
      }
#   if (defined (__WINDOWS__))
    return (win_error (libHandle,rc));
#   else
    return (rc);
#   endif
#else
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_peer_addr

    Synopsis: Builds an address block (struct sockaddr_in) for the remote
    end of the specified connected socket.  Returns 0 if okay, SOCKET_ERROR
    if there was an error.  If the name argument is not null, looks-up the
    host name and returns it.  The name is truncated to namesize characters,
    including a trailing null character.
    ---------------------------------------------------------------------[>]-*/

int
get_peer_addr (
    TA_Libc *libHandle,
    sock_t handle,                      /*  Socket to get address for        */
    struct sockaddr_in *sin,            /*  Block for formatted address      */
    char *name,                         /*  Buffer for host name, or NULL    */
    int namesize                        /*  Size of host name buffer         */
)
{
#if (defined (DOES_SOCKETS))
    int
        rc;                             /*  Return code from call            */
    struct hostent
        *phe;                           /*  Host information entry           */
    argsize_t
        sin_length;                     /*  Length of address                */

    TA_ASSERT_RET (libHandle,sin,SOCKET_ERROR);

    /*  Get address for connected socket peer                                */
    sin_length = sizeof (struct sockaddr_in);
    rc = getpeername ((SOCKET) handle, (struct sockaddr *) sin, &sin_length);

    /*  Translate into host name string, only if wanted                      */
    if (name != NULL && rc == 0)
      {
        phe = gethostbyaddr ((char *) &sin-> sin_addr,
                             sizeof (sin-> sin_addr), AF_INET);
        if (phe)
          {
            strncpy (name, phe-> h_name, namesize);
            name [namesize - 1] = '\0';
          }
      }
#   if (defined (__WINDOWS__))
    return (win_error (libHandle,rc));
#   else
    return (rc);
#   endif
#else
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: read_TCP

    Synopsis:
    Reads data from the socket.  On UNIX, VMS, OS/2, passes through to the
    standard read function; some other systems have particular ways of
    accessing sockets.  If there is an error on the read this function
    returns SOCKET_ERROR.  You can handle errors (in sockerrno) as fatal except
    for EAGAIN which indicates that the operation would cause a non-blocking
    socket to block, and EPIPE or ECONNRESET which indicate that the socket
    was closed at the other end.  Treat EWOULDBLOCK as EAGAIN.
    ---------------------------------------------------------------------[>]-*/

int
read_TCP (
    TA_Libc *libHandle,
    sock_t handle,                      /*  Socket handle                    */
    void *buffer,                       /*  Buffer to receive data           */
    size_t length                       /*  Maximum amount of data to read   */
)
{
#if (defined (DOES_SOCKETS))
#   if (defined (__UTYPE_BEOS))
    return (recv ((SOCKET) handle, buffer, length, 0));

#   elif (defined (__UNIX__) || defined (__VMS__) || defined (__OS2__))
    return (read ((SOCKET) handle, buffer, length));

#   elif (defined (__WINDOWS__))
    int
        rc;                             /*  Return code from call            */

    TA_ASSERT_RET (libHandle,buffer,SOCKET_ERROR);
    rc = recv ((SOCKET) handle, buffer, length, 0);
    return (win_error (libHandle,rc));
#   else
#       error "No code for function body."
#   endif
#else
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


#if (defined (__WINDOWS__))
/*  -------------------------------------------------------------------------
 *  win_error -- internal
 *
 *  For Winsockets only: fetches real error code and sticks it in errno,
 *  if the return code from the last call was SOCKET_ERROR.  Returns rc.
 */

static int
win_error (TA_Libc *libHandle, int rc)
{
    if (rc == (int) SOCKET_ERROR)
        errno = winsock_last_error (libHandle);        

    return (rc);
}
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: write_TCP

    Synopsis:
    Writes data to the socket.  On UNIX, VMS, OS/2, calls the standard
    write function; some other systems have particular ways of accessing
    sockets.  If there is an error on the write this function returns
    SOCKET_ERROR.  You can handle errors (in sockerrno) as fatal except for
    EAGAIN which indicates that the operation would cause a non-blocking
    socket to block, and EPIPE or ECONNRESET which indicate that the socket
    was closed at the other end.  Treat EWOULDBLOCK as EAGAIN.
    ---------------------------------------------------------------------[>]-*/

int
write_TCP (
    TA_Libc *libHandle,
    sock_t handle,                      /*  Socket handle                    */
    const void *buffer,                 /*  Buffer containing data           */
    size_t length                       /*  Amount of data to write          */
)
{
#if (defined (DOES_SOCKETS))
#   if (defined (__UTYPE_BEOS))
    (void)libHandle;
    return (send ((SOCKET) handle, buffer, length, 0));

#   elif (defined (__UNIX__) || defined (__VMS__) || defined (__OS2__))
    (void)libHandle;
    return (write ((SOCKET) handle, buffer, length));

#   elif (defined (__WINDOWS__))
    int
        rc;                             /*  Return code from call            */

    TA_ASSERT_RET (libHandle,buffer,SOCKET_ERROR);
    rc = send ((SOCKET) handle, buffer, length, 0);
    return (win_error (libHandle,rc));
#   else
#       error "No code for function body."
#   endif
#else
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: read_UDP

    Synopsis:
    Reads data from a connected or unconnected UDP socket.  To prepare a
    connected UDP socket you call connect_UDP ().  This makes a connection
    to a specific port on a specific host, and returns a socket handle.
    When you call this function with a null value for the address argument,
    it assumes you are using a connected UDP socket.

    To prepare an unconnected UDP socket, call create_socket () with the
    string "udp" as argument.  This returns a sock_t handle that you can
    use in this function.  If you use an unconnected UDP socket you must
    provide an address structure.  The function places the remote host and
    port in this structure.  This lets you reply using write_UDP ().

    Generally a server can use unconnected sockets, and a client can use
    connected sockets.  You can also format an address for a specific host
    and port using the address_end_point () function.

    If there is an error on the read this function returns SOCKET_ERROR.
    You can handle errors (in sockerrno) as fatal except for EAGAIN which
    indicates that the operation would cause a non-blocking socket to block.
    Treat EWOULDBLOCK as EAGAIN.
    ---------------------------------------------------------------------[>]-*/

int
read_UDP (
    TA_Libc *libHandle,
    sock_t handle,                      /*  Socket handle                    */
    void *buffer,                       /*  Buffer to receive data           */
    size_t length,                      /*  Maximum amount of data to read   */
    const struct sockaddr_in *sin       /*  Block for address, or null       */
)
{
#if (defined (DOES_SOCKETS))
    argsize_t
        sin_length;                     /*  Length of address                */
    int
        flags = 0,                      /*  Flags for call                   */
        rc;                             /*  Return code from call            */

    TA_ASSERT_RET (libHandle,buffer,SOCKET_ERROR);
    TA_ASSERT_RET (libHandle,sin,SOCKET_ERROR);
    sin_length = (int) sizeof (*sin);
    if (sin)
        /*  Read from unconnected UDP socket; we accept the address of the   */
        /*  sending party in the sin argument.                               */
        rc = recvfrom ((SOCKET) handle, buffer, length, flags,
                      (struct sockaddr *) sin, &sin_length);
    else
        /*  Read from a connected UDP socket; we don't need to get the       */
        /*  address, since we already know it.                               */
        rc = recv     ((SOCKET) handle, buffer, length, flags);

#   if (defined (__WINDOWS__))
    return (win_error (libHandle,rc));
#   else
    return (rc);
#   endif
#else
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: write_UDP

    Synopsis:
    Writes data to a connected or unconnected UDP socket.  To prepare a
    connected UDP socket you call connect_UDP ().  This makes a connection
    to a specific port on a specific host, and returns a socket handle.
    When you call this function with a null value for the address argument,
    it assumes you are using a connected UDP socket.

    To prepare an unconnected UDP socket, call create_socket () with the
    string "udp" as argument.  This returns a sock_t handle that you can
    use in this function.  If you use an unconnected UDP socket you must
    provide an address structure containing a valid host and port.  You can
    get this information from a read_UDP () or through address_end_point ().

    If there is an error on the write this function returns SOCKET_ERROR.
    You can handle errors as fatal except for EAGAIN which indicates that
    the operation would cause a non-blocking socket to block.  Treat
    EWOULDBLOCK as EAGAIN.
    ---------------------------------------------------------------------[>]-*/

int
write_UDP (
    TA_Libc *libHandle,
    sock_t handle,                      /*  Socket handle                    */
    const void *buffer,                 /*  Buffer containing data           */
    size_t length,                      /*  Amount of data to write          */
    const struct sockaddr_in *sin       /*  Address to send to, or null      */
)
{
#if (defined (DOES_SOCKETS))
    int
        sin_length,                     /*  Length of address                */
        flags = 0,                      /*  Flags for call                   */
        rc;                             /*  Return code from call            */

    TA_ASSERT_RET (libHandle,buffer,SOCKET_ERROR);
    TA_ASSERT_RET (libHandle,sin,SOCKET_ERROR);
    sin_length = (int) sizeof (*sin);
    if (sin)
        /*  Write to unconnected UDP socket; we provide the address of       */
        /*  the receiving party in the sin argument.                         */
        rc = sendto ((SOCKET) handle, buffer, length, flags,
                    (struct sockaddr *) sin, sin_length);
    else
        /*  Write to a connected UDP socket; we don't need to supply         */
        /*  the address, since we already know it.                           */
        rc = send   ((SOCKET) handle, buffer, length, flags);

#   if (defined (__WINDOWS__))
    return (win_error (libHandle,rc));
#   else
    return (rc);
#   endif
#else
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: close_socket

    Synopsis:
    Closes the socket.  On UNIX, VMS, OS/2 calls the standard close
    function; some other systems have particular ways of accessing sockets.
    If there is an error on the close this function returns SOCKET_ERROR.
    You can handle errors (in sockerrno) as fatal except for EAGAIN which
    indicates that the operation would cause a non-blocking socket to block.
    Treat EWOULDBLOCK as EAGAIN.
    ---------------------------------------------------------------------[>]-*/

int
close_socket (
    TA_Libc *libHandle,
    sock_t handle                       /*  Socket handle                    */
)
{
#if (defined (DOES_SOCKETS))
#   if (defined (__UNIX__) || defined (__VMS__) || defined (__OS2__))
    if (!socket_is_alive (libHandle,handle))
        return (0);
    ip_sockets--;
        shutdown (handle, 2); 
    return (close ((SOCKET) handle));

#   elif (defined (__WINDOWS__))
    int
        rc;

    if (!socket_is_alive (libHandle,handle))
        return (0);
    ip_sockets--;
        shutdown ((SOCKET) handle, 2); 
    rc = closesocket ((SOCKET) handle);
    return (win_error (libHandle,rc));

#   else
#       error "No code for function body."
#   endif

#elif (defined (FAKE_SOCKETS))
    return (0);                         /*  Okay, closed                     */

#else
    return ((int) SOCKET_ERROR);        /*  Sockets not supported            */
#endif
}

/*  ---------------------------------------------------------------------[<]-
    Function: sock_select

    Synopsis: Performs the standard select() call.  Use this in preference
    to select(), as some systems may not be 100% compatible with BSD sockets,
    Uses the same arguments as the select() call, and gives the same return
    codes.  If sockets are not supported, always returns 0.
    ---------------------------------------------------------------------[>]-*/

int
sock_select (TA_Libc *libHandle, int nfds, fd_set *readfds, fd_set *writefds,
             fd_set *errorfds, struct timeval *timeout)
{
#if (defined (DOES_SOCKETS))
    int
        rc = 0;                         /*  Return code from select()        */
    TA_ASSERT_RET (libHandle,timeout,0);

#   if (defined (__UTYPE_BEOS))
    /*  BeOS only supports the readfds argument                              */
    rc = select (nfds, FD_SETTYPE readfds, NULL, NULL, timeout);
/*    if (rc == -1)
        coprintf ("Error after select(): %s", strerror (errno)); */
    return (rc);
    
#   elif (defined (WIN32))
    /*  Windows occasionally aborts during the select call...                */
/*    __try {*/
        rc = select (nfds, FD_SETTYPE readfds, FD_SETTYPE writefds,
                           FD_SETTYPE errorfds, timeout);
/*    }
    __except (1) {*/
/*        coprintf ("select() aborted - arguments: %d, %p, %p, %p, %p",
                     nfds, FD_SETTYPE readfds, FD_SETTYPE writefds,
                           FD_SETTYPE errorfds, timeout);*/
/*    }*/
    return (rc);

#   else
    rc = select (nfds, FD_SETTYPE readfds, FD_SETTYPE writefds,
                       FD_SETTYPE errorfds, timeout);
    return (rc);
#   endif
#else
    return (0);
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_hostname

    Synopsis: Returns a string containing the local hostname.  The returned
    string is in a static area.  Only performs the local hostname lookup one
    time; the returned value is cached for later repeated calls to this
    function.  If sockets are not supported, returns the value "localhost".
    ---------------------------------------------------------------------[>]-*/

char *
get_hostname (TA_Libc *libHandle)
{
#if (defined (DOES_SOCKETS))
    static char
        host_name [LINE_MAX + 1] = "";

    (void)libHandle; /* Get ride of compiler warnings. */

    if (strnull (host_name))
        if (gethostname (host_name, LINE_MAX))
            strcpy (host_name, "localhost");
    return (host_name);
#else
    return ("localhost");
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_hostaddr

    Synopsis: Returns the current the host address as a 4-byte value in
    host format (not network format).  Returns 0x7f000001 (loopback) if
    sockets are not supported or there was an error getting the current
    host IP address.  If there are several IP addresses on the system,
    returns one arbitrary address.
    ---------------------------------------------------------------------[>]-*/

qbyte
get_hostaddr (TA_Libc *libHandle)
{
#if (defined (DOES_SOCKETS))
    struct hostent
        *phe;                           /*  Host information entry           */

    phe = gethostbyname (get_hostname (libHandle));
    if (phe)
        return (*(qbyte *) (phe-> h_addr_list [0]));
    else
        return (htonl (SOCKET_LOOPBACK));
#else
    return (htonl (SOCKET_LOOPBACK));
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_hostaddrs

    Synopsis: Returns a table of all host IP addresses.  The table ends in
    a zero address.  Each address is a 4-byte value in host format.  Returns
    NULL if there was an error.  If sockets are not supported, returns a
    table with the loopback address (127.0.0.1) and a null address.  The
    caller must free the table using TA_Free( libHandle, () when finished using it.
    ---------------------------------------------------------------------[>]-*/

qbyte *
get_hostaddrs (TA_Libc *libHandle)
{
#if (defined (DOES_SOCKETS))
    int
        addr_count;                     /*  How many addresses do we have    */
    qbyte
        *addr_table;                    /*  Where we store the addresses     */
    struct hostent
        *phe;                           /*  Host information entry           */

    if ((phe = gethostbyname (get_hostname (libHandle))) == NULL)
        return (NULL);

    /*  Count the addresses                                                  */
    for (addr_count = 0; phe-> h_addr_list [addr_count]; addr_count++);

    /*  Allocate a table; socket addresses are 4 bytes                       */
    addr_table = TA_Malloc( libHandle,  (4 * (addr_count + 1)));

    /*  Store the addresses                                                  */
    for (addr_count = 0; phe-> h_addr_list [addr_count]; addr_count++)
        addr_table [addr_count]
            = *(qbyte *) (phe-> h_addr_list [addr_count]);

    addr_table [addr_count] = 0;
    return (addr_table);

#else
    qbyte
        *addr_table;                    /*  Where we store the addresses     */

    addr_table = TA_Malloc( libHandle,  (8);         /*  Addresses are 4 bytes            */
    addr_table [0] = htonl (SOCKET_LOOPBACK);
    addr_table [1] = 0;
    return (addr_table);
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: sock_ntoa

    Synopsis: Converts an IP address in network order to a string in dotted
    format.  The string is stored in a statically-allocated buffer that is
    overwritten by each call.
    ---------------------------------------------------------------------[>]-*/

char *
sock_ntoa (TA_Libc *libHandle,qbyte address)
{
    static char
        string [16];                    /*  xxx.xxx.xxx.xxx                  */
    byte
        *part;

    (void)libHandle; /* Get ride of compiler warnings. */

    /*  Network order is high-low so we can address the bytes in order       */
    part = (byte *) &address;
    snprintf (string, sizeof (string), 
              "%d.%d.%d.%d", part [0], part [1], part [2], part [3]);
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: sockmsg

    Synopsis:
    Returns a string describing the cause of the last fatal error to occur
    a socket.  Should be called directly after a socket i/o operation; if you
    do other i/o operations or allow other threads to proceed in the meantime,
    the returned string may be incorrect.
    ---------------------------------------------------------------------[>]-*/

const char *
sockmsg (TA_Libc *libHandle)
{
#if (defined (__WINDOWS__))
    char
        *message;

    (void)libHandle; /* Get ride of compiler warnings. */

    switch (WSAGetLastError ())
      {
        case WSAEINTR:           message = "WSAEINTR";           break;
        case WSAEBADF:           message = "WSAEBADF";           break;
        case WSAEACCES:          message = "WSAEACCES";          break;
        case WSAEFAULT:          message = "WSAEFAULT";          break;
        case WSAEINVAL:          message = "WSAEINVAL";          break;
        case WSAEMFILE:          message = "WSAEMFILE";          break;
        case WSAEWOULDBLOCK:     message = "WSAEWOULDBLOCK";     break;
        case WSAEINPROGRESS:     message = "WSAEINPROGRESS";     break;
        case WSAEALREADY:        message = "WSAEALREADY";        break;
        case WSAENOTSOCK:        message = "WSAENOTSOCK";        break;
        case WSAEDESTADDRREQ:    message = "WSAEDESTADDRREQ";    break;
        case WSAEMSGSIZE:        message = "WSAEMSGSIZE";        break;
        case WSAEPROTOTYPE:      message = "WSAEPROTOTYPE";      break;
        case WSAENOPROTOOPT:     message = "WSAENOPROTOOPT";     break;
        case WSAEPROTONOSUPPORT: message = "WSAEPROTONOSUPPORT"; break;
        case WSAESOCKTNOSUPPORT: message = "WSAESOCKTNOSUPPORT"; break;
        case WSAEOPNOTSUPP:      message = "WSAEOPNOTSUPP";      break;
        case WSAEPFNOSUPPORT:    message = "WSAEPFNOSUPPORT";    break;
        case WSAEAFNOSUPPORT:    message = "WSAEAFNOSUPPORT";    break;
        case WSAEADDRINUSE:      message = "WSAEADDRINUSE";      break;
        case WSAEADDRNOTAVAIL:   message = "WSAEADDRNOTAVAIL";   break;
        case WSAENETDOWN:        message = "WSAENETDOWN";        break;
        case WSAENETUNREACH:     message = "WSAENETUNREACH";     break;
        case WSAENETRESET:       message = "WSAENETRESET";       break;
        case WSAECONNABORTED:    message = "WSAECONNABORTED";    break;
        case WSAECONNRESET:      message = "WSAECONNRESET";      break;
        case WSAENOBUFS:         message = "WSAENOBUFS";         break;
        case WSAEISCONN:         message = "WSAEISCONN";         break;
        case WSAENOTCONN:        message = "WSAENOTCONN";        break;
        case WSAESHUTDOWN:       message = "WSAESHUTDOWN";       break;
        case WSAETOOMANYREFS:    message = "WSAETOOMANYREFS";    break;
        case WSAETIMEDOUT:       message = "WSAETIMEDOUT";       break;
        case WSAECONNREFUSED:    message = "WSAECONNREFUSED";    break;
        case WSAELOOP:           message = "WSAELOOP";           break;
        case WSAENAMETOOLONG:    message = "WSAENAMETOOLONG";    break;
        case WSAEHOSTDOWN:       message = "WSAEHOSTDOWN";       break;
        case WSAEHOSTUNREACH:    message = "WSAEHOSTUNREACH";    break;
        case WSAENOTEMPTY:       message = "WSAENOTEMPTY";       break;
        case WSAEPROCLIM:        message = "WSAEPROCLIM";        break;
        case WSAEUSERS:          message = "WSAEUSERS";          break;
        case WSAEDQUOT:          message = "WSAEDQUOT";          break;
        case WSAESTALE:          message = "WSAESTALE";          break;
        case WSAEREMOTE:         message = "WSAEREMOTE";         break;
        case WSAEDISCON:         message = "WSAEDISCON";         break;
        case WSASYSNOTREADY:     message = "WSASYSNOTREADY";     break;
        case WSAVERNOTSUPPORTED: message = "WSAVERNOTSUPPORTED"; break;
        case WSANOTINITIALISED:  message = "WSANOTINITIALISED";  break;
        default:                 message = "No error";
      }
    return (message);
#else
    return (strerror (errno));
#endif
}


#if (defined (__WINDOWS__))
/*  ---------------------------------------------------------------------[<]-
    Function: winsock_last_error

    Synopsis: Convert a winsock error into a errno value.
    ---------------------------------------------------------------------[>]-*/

int
winsock_last_error (TA_Libc *libHandle)
{
    int
        error = 0;
    int temp;

    (void)libHandle; /* Get ride of compiler warnings. */

    temp = WSAGetLastError ();
    switch (temp)
      {
        case WSAEINTR:           error = EINTR;           break;
        case WSAEBADF:           error = EBADF;           break;
        case WSAEWOULDBLOCK:     error = EAGAIN;          break;
        case WSAEINPROGRESS:     error = EAGAIN;          break;
        case WSAENETDOWN:        error = EAGAIN;          break;
        case WSAECONNRESET:      error = ECONNRESET;      break;
        case WSAECONNABORTED:    error = EPIPE;           break;
        case WSAESHUTDOWN:       error = ECONNRESET;      break;
        case WSAEINVAL:          error = EPIPE;           break;
#  if defined (WIN32)
        default:                 error = GetLastError ();
#  else
        default:                 error = errno;
#  endif
      }
    return (error);
}
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: socket_is_permitted

    Synopsis: Compares the specified address with a mask and returns
    TRUE if the address matches the mask, or FALSE if it does not.  The
    address is formatted as a string "xxx.xxx.xxx.xxx".  The mask is
    formatted as zero or more patterns, delimited by whitespace or commas.
    A pattern is an address string, with zero or more of the last
    components replaced by '*'.  The pattern may also be prefixed by '!'
    to indicate exclusion.  This is an example of a mask: "127.0.0.1,
    253.34.*, !253.35.*".  This mask allows all addresses: "*".  To get
    the string address for a remote socket, use socket_peer_address().
    ---------------------------------------------------------------------[>]-*/

Bool
socket_is_permitted (TA_Libc *libHandle,const char *address, const char *mask)
{
    char
        *addrptr,                       /*  Pointer into address             */
        *maskptr;                       /*  Pointer into mask                */
    Bool
        negate,                         /*  If !pattern                      */
        feedback = FALSE;               /*  False unless matched             */

    TA_ASSERT_RET (libHandle,address,FALSE);
    TA_ASSERT_RET (libHandle,mask,FALSE);

    maskptr = (char *) mask;
    while (*maskptr)
      {
        while (isspace (*maskptr) || *maskptr == ',')
            maskptr++;

        /*  Get negation if necessary                                        */
        if (*maskptr == '!')
          {
            negate = TRUE;
            maskptr++;
          }
        else
            negate = FALSE;

        /*  Compare pattern with address up to the end of the pattern        */
        for (addrptr = (char *) address; *addrptr; addrptr++)
          {
            if (*maskptr == '*')        /*  Matched address up to *          */
                return ((Bool)!negate);       /*  So either accepted or failed     */
            else
            if (*maskptr == '\0')       /*  Did not match address            */
                return (negate);        /*    so fail unless negated         */
            else
            if (*addrptr != *maskptr)   /*  Some difference                  */
                break;                  /*    so stop comparing              */

            maskptr++;
          }
        if (*addrptr == '\0'            /*  Matched exact address?           */
        && (*maskptr == '\0' || isspace (*maskptr) || *maskptr == ','))
            return ((Bool)!negate);           /*  Either accepted or failed        */

        until (*maskptr == '\0' || isspace (*maskptr) || *maskptr == ',')
            maskptr++;                  /*  Skip to end of this pattern      */
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_host_file

    Synopsis: returns the full path name of the host lookup file, if provided
    by the OS, and found.  If not found, returns "hosts".  The returned string
    is held in a static area of memory that may be overwritten by each call.
    ---------------------------------------------------------------------[>]-*/

char *
get_host_file (TA_Libc *libHandle)
{
#if (defined (WIN32))
    static OSVERSIONINFO
        version_info;
    static char
        name [LINE_MAX + 1];

    (void)libHandle;

    strclr (name);
    GetWindowsDirectory (name, LINE_MAX);
    version_info.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (GetVersionEx (&version_info))
        /*  On Windows NT the hosts file is well-hidden; on Win95 it's
         *  more visible                                                     */
        if (version_info.dwPlatformId == VER_PLATFORM_WIN32_NT)
            strcat (name, "\\system32\\drivers\\etc\\hosts");
        else
            strcat (name, "\\hosts");
    return (name);

#elif (defined (__UNIX__))
    return ("/etc/hosts");

#elif (defined (__VMS__))
    return ("/etc/hosts");              /*  Not correct -- needs more work   */

#elif (defined (__OS2__))
    /*  Under OS/2 the hosts information is stored in the "hosts" file which
     *  is in the directory pointed at by the %ETC% environment variable.
     *  If that environment variable is not set, then TCP/IP support is not
     *  properly installed.  In that instance we return "/mptn/etc/hosts"
     *  (the likely value on OS/2 Warp 3 Connect and OS/2 Warp 4) and hope
     *  for the best.
     */

    /*  A static array is used only because the other versions use a static
     *  array.  If the resulting file name will not fit in the space allowed
     *  then "/mtpn/etc/hosts" is used as before.
     */
    static char
        name [LINE_MAX + 1];
    char
        *etcenv = NULL;

    etcenv = getenv ("ETC");
    if (etcenv != NULL && strlen (etcenv) < (LINE_MAX - 6))
      { /*  We've already checked it will all fit.                           */
        strcpy (name, etcenv);
        strcat (name, "/hosts");
        return (name);
      }
    else
        return ("/mptn/etc/hosts");

#else
    return ("hosts");
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_name_server

    Synopsis: gets the addresses of the DNS servers defined in the TCP/IP
    configuration.  The addresses are returned in a user-provided struct
    sockaddr_in array.  The maximum number of addresses in this array is
    supplied as the ns_max argument.  Return the number of address found.
    ---------------------------------------------------------------------[>]-*/

int
get_name_server (TA_Libc *libHandle,struct sockaddr_in *ns_address, int ns_max)
{
    int
        ns_count = 0;                /*  Number of servers that we found  */

#if (defined (WIN32))
    static OSVERSIONINFO
        version_info;
    HKEY
        hkey;                           /*  Handle to returned reg. key      */
    static char
        registry_value [LINE_MAX + 1];  /*  DNS server info from registry    */
    long
        size = LINE_MAX;                /*  Max. size of returned value      */
    DWORD
        type;
    char
        *key,
        **address_list = NULL;
    int
        address_nbr;

    /*  Look in registry; this sometimes works, but not always               */
    version_info.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (GetVersionEx (&version_info)
    &&  version_info.dwPlatformId == VER_PLATFORM_WIN32_NT)
        key = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
    else
        key = "SYSTEM\\CurrentControlSet\\Services\\Vxd\\Mstcp\\Parameters";

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, key, 0,
        KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS
    &&  RegQueryValueEx (hkey, "NameServer", NULL, (LPDWORD) &type,
        (LPBYTE) registry_value, (LPDWORD) &size) == ERROR_SUCCESS)
      {
        address_list = tok_split (libHandle,registry_value);
        for (address_nbr = 0; address_list [address_nbr]; address_nbr++)
          {
            if (ns_count >= ns_max)
                break;

            ns_address [ns_count].sin_family      = AF_INET;
            ns_address [ns_count].sin_port        = htons (DNS_PORT);
            ns_address [ns_count].sin_addr.s_addr =
                inet_addr (address_list [address_nbr]);
            ns_count++;
          }
        tok_free (libHandle,address_list);
        RegCloseKey (hkey);
      }

#elif (defined (__UNIX__))
    static char
        buffer  [LINE_MAX + 1],
        address [16];
    FILE
        *resolver;
    int
        rc;

    resolver = file_open (libHandle,"/etc/resolv.conf", 'r');
    if (resolver)
      {
        while (file_read (libHandle,resolver, buffer))
          {
            rc = sscanf (buffer, "nameserver %s", address);
            if (rc > 0 && rc != EOF)
              {
                if (ns_count >= ns_max)
                    break;

                ns_address [ns_count].sin_family      = AF_INET;
                ns_address [ns_count].sin_port        = htons (DNS_PORT);
                ns_address [ns_count].sin_addr.s_addr = inet_addr (address);
                ns_count++;
              }
          }
        file_close (libHandle,resolver);
      }

#elif (defined (__OS2__))
    static char
        buffer  [LINE_MAX + 1],
        address [16];
    char
        *etcenv   = NULL,
        *filename = NULL;
    FILE
        *resolver = NULL;
    int
        rc;

    /*  Under OS/2 the file controlling the resolver is stored in the        */
    /*  directory pointed at by the ETC environment variable.  It is called  */
    /*  resolv2 or resolv (I *think* that is the order of preference), so we */
    /*  try those two file names in that order.                              */

    /*  If the ETC environment variable is not set we try the /mptn/etc      */
    /*  directory since that is a likely default location for it.            */

    etcenv = getenv ("ETC");
    if (etcenv)
      {
        filename = TA_Malloc( libHandle,  (strlen(etcenv) + 10);
        if (!filename)
          return 0;                  /*  Cannot allocate memory for filename */

        strcpy (filename, etcenv);
        strcat (filename, "/resolv2");

        resolver = file_open (filename, 'r');

        if (! resolver)
          { /*  Not available under that filename, let's try the other one   */
            strcpy (filename, etcenv);
            strcat (filename, "/resolv");

            resolver = file_open (filename, 'r');
          }
        TA_Free( libHandle,  (filename);
      }
    else
      { /*  No environment variable around, try using the defaults           */
        resolver = file_open ("/mptn/etc/resolv2", 'r');
        if (! resolver)
            resolver = file_open ("/mptn/etc/resolv", 'r');
      }
    if (resolver)
      {
        while (file_read (resolver, buffer))
          {
            rc = sscanf (buffer, "nameserver %s", address);
            if (rc > 0 && rc != EOF)
              {
                if (ns_count >= ns_max)
                    break;

                ns_address [ns_count].sin_family      = AF_INET;
                ns_address [ns_count].sin_port        = htons (DNS_PORT);
                ns_address [ns_count].sin_addr.s_addr = inet_addr (address);
                ns_count++;
              }
          }
        file_close (resolver);
      }
#endif

    return (ns_count);
}


/*  We fake some BSD functions that are missing on some systems              */
#if (defined (__VMS__) || defined (__UTYPE_BEOS))
struct protoent *getprotobyname (TA_Libc *libHandle,const char *protocol)
{
    static struct protoent
        proto = { NULL, NULL, 0 };

    if (streq (protocol, "udp"))
        proto.p_proto = IPPROTO_UDP;
    else
    if (streq (protocol, "tcp"))
        proto.p_proto = IPPROTO_TCP;
    else
    if (streq (protocol, "icmp"))
        proto.p_proto = IPPROTO_ICMP;

    return (&proto);
}

struct protoent *getprotobynumber (TA_Libc *libHandle,int protocol)
{
    static struct protoent
        proto = { NULL, NULL, 0 };

    if (protocol == IPPROTO_UDP)
        proto.p_name = "udp";
    else
    if (protocol == IPPROTO_TCP)
        proto.p_name = "tcp";
    else
    if (protocol == IPPROTO_ICMP)
        proto.p_name = "icmp";

    return (&proto);
}

struct servent  *getservbyport (TA_Libc *libHandle,int port, const char *protocol)
{
    static char
        fmtnumber [9];
    static struct servent
        service = { NULL, NULL, 0, NULL };
    sprintf (fmtnumber, "%d", port);
    service.s_name = fmtnumber;
    return (&service);
}
#endif

