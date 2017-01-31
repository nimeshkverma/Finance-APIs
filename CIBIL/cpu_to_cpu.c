#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
int s; /* connected socket descriptor */
struct hostent* hp; /* pointer to host info for remote host */
struct servent* sp; /* pointer to service information */
long timevar; /* contains time returned by time() */
char* ctime(); /* declare time formatting routine */
struct sockaddr_in myaddr_in; /* for local socket address */
struct sockaddr_in peeraddr_in; /* for peer socket address */
char* tstuef = "TUEF11 BP0393111233XXXXX000150000 021PP";
char* tstuef2 = "PN03N010108A BATTER07080101197008011";
char* tstuef3 = "PA03A010108DIRTY RD0207CHENNAI0602330706600015";
char* tstuef4 = "ES05001660102**";
/**************************************************************
MAIN
This routine is the client which request service from the remote
"CIBIL_enquiry" server. It creates a connection, sends a number of
requests, shuts down the connection in one direction to signal the
server about the end of data, and then receives all of the
responses. Status will be written to stdout.
The name of the system to which the requests will be sent is
given as a parameter to the command
**************************************************************/
main(argc, argv) int argc;
char* argv[];
{
    FILE *fopen(), *fplog, *infp;
    int bufcnt, addrlen, i, j, trans_count = 0;
    int buflen;
    char buf[32768];
    fplog = fopen("TEST_LOG", "w");
    /* clear out address structures */
    memset((char*)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset((char*)&peeraddr_in, 0, sizeof(struct sockaddr_in));
    /* We will connect with the internet address family */
    peeraddr_in.sin_family = AF_INET;
    /********************************************************
      Get the host information (from the "etc/hosts" file)
      for the hostname that the user passed in
    ********************************************************/
    hp = gethostbyname("cibil_prod"); /* get proper ip address entry from CIBIL */
    if (hp == NULL) {
        fprintf(fplog, "%s: %s not found in /etc/hosts\n", argv[0], "cibil_prod");
        fflush(fplog);
        exit(1);
    }
    peeraddr_in.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr))->s_addr;
    fprintf(fplog, "host name = %s\n", hp->h_name);
    fprintf(fplog, "host alias = %s\n", hp->h_aliases[0]);
    fprintf(fplog, "host address type = %s\n", hp->h_addrtype);
    fflush(fplog);
    /*******************************************************
      Find the information for the "cibil_enquiry" server
      in order to get the needed port number. Found in
      etc/services
    *******************************************************/
    /* get proper entry from CIBIL should be 25009 */
    sp = getservbyname("cibil_enquiry", "tcp");
    if (sp == NULL) {
        fprintf(fplog, "%s: cibil_enquiry not found in /etc/services\n", argv[0]);
        fflush(fplog);
        exit(1);
    }
    peeraddr_in.sin_port = sp->s_port;
    fprintf(fplog, "service name = %s\n", sp->s_name);
    fprintf(fplog, "service protocol = %s\n", sp->s_proto);
    fflush(fplog);
    /* Create the socket */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perror(argv[0]);
        fprintf(fplog, "%s: unable to create socket\n", argv[0]);
        fflush(fplog);
        exit(1);
    }
    fprintf(fplog, "created a socket = %d\n", s);
    fflush(fplog);
    /******************************************************
      Try to connect to the remote server at the address
      which was just built into peeraddr
    ******************************************************/
    if (connect(s, &peeraddr_in, sizeof(struct sockaddr_in)) == -1) {
        perror(argv[0]);
        fprintf(fplog, "%s: unable to connect to remote\n", argv[0]);
        fflush(fplog);
        exit(1);
    }
    fprintf(fplog, "SUCCESSFUL connect to the remote server\n");
    fflush(fplog);
    /******************************************************
      Since the connect call assigns a random address
      to the local end of this connection, let's use
      getsockname to see what it assigned. Note that
      addrlen needs to be passed in as a pointer,
      because getsockname returns the acutal length
      of the address
    ******************************************************/ 
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, &myaddr_in, &addrlen) == -1) {
        perror(argv[0]);
        fprintf(fplog, "%s: unable to read socket address\n", argv[0]);
        fflush(fplog);
        exit(1);
    }
    /******************************************************
      The port number must be converted first to host byte
      order before printing. On most hosts, this is not
      necessary, but the ntohs() call is included here so
      that this program could easily be ported to a host
      that does require it
    ******************************************************/
    time(&timevar);
    fprintf(fplog, "Connected to %s on port %u (%u) at %s\n",
        "TUprod", ntohs(myaddr_in.sin_port),
        myaddr_in.sin_port, ctime(&timevar));
    fflush(fplog);
    while (1) {
        /* Sent out the request to the remote server */
        test_data(buf);
        buflen = strlen(buf);
        buf[buflen++] = 0x13;
        buf[buflen] = 0;
        if (send(s, buf, buflen, 0) == -1) {
            fprintf(fplog, "%s: Connection aborted on send error\n", argv[0]);
            fflush(fplog);
            exit(1);
        }
        fprintf(fplog, "Sent data length = %d\n", buflen);
        fflush(fplog);
        /*******************************************************
          Now, start receiving the reply from the server.
          This loop will terminate when the recv returns zero
          which is an end-of-file condition, OR when an end
          of transmission char is received
        *******************************************************/
        bufcnt = 0;
        while (1) {
            i = recv(s, &buf[bufcnt], 10, 0);
            if (i == -1) {
                perror(argv[0]);
                fprintf(fplog, "%s: error reading result\n", argv[0]);
                fflush(fplog);
                exit(1);
            }
            bufcnt += i;
            fprintf(fplog, "REceived length = %d\n", bufcnt);
            fflush(fplog);
            if (i == 0) {
                break;
            }
            /* Check for terminator */
            if (buf[bufcnt - 1] == 0x13) {
                fprintf(fplog, "GOT TERMINATOR\n");
                fprintf(fplog, "trans count = %d\n", trans_count++);
                fprintf(fplog, "%s", buf);
                fflush(fplog);
                break;
            }
        }
        fprintf(fplog, "Received data:\n<%s>\n\n", buf);
        if (trans_count >= 10) {
            break;
        }
    }
    /****************************************************
       shutdown the connection for further sends.
      This will cause the server to receive an end-of-file
      condition after it has received all the requests that
      have just been sent, indicating that we will not be
      sending any further requests.
    ****************************************************/
    if (shutdown(s, 1) == -1) {
        perror(argv[0]);
        fprintf(fplog, "%s: unable to shutdown socket\n", argv[0]);
        fflush(fplog);
    }
    close(s);
    /* Print mesage indicating completion of task */
    time(&timevar);
    fprintf(fplog, "All done at %s", ctime(&timevar));
    fflush(fplog);
}
test_data(char* buffer)
{
    strcpy(buffer, tstuef);
    strcat(buffer, tstuef2);
    strcat(buffer, tstuef3);
    strcat(buffer, tstuef4);
}