#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <algorithm> 
#include <iostream> 
#include <fstream> 
#include <string>
#include <sstream>
#include <errno.h>
#include <sys/time.h>
#include <string>
#include "openthread/thread_ftd.h"
#include "common/logging.hpp"
#include "agent/rest_web_server.hpp"

#include "cJSON.h"
#include "cJSON.c"
#include "http_parser.h"
#include "http_parser.c"



namespace otbr {
namespace agent {


static int on_message_begin(http_parser *parser) {
    Connection *Connection = (struct Connection*)parser->data;
    
    otbrLog(OTBR_LOG_ERR, "complete: %d",Connection->complete );
    return 0;
}

static int on_status(http_parser *parser,const char *at, size_t len) {
    Connection *Connection = (struct Connection*)parser->data;
    Connection->status = new char[len+1]();
   
    
    strncpy(Connection->status,at,len);
    // Connection->status[0] = '\0';
    // otbrLog(OTBR_LOG_ERR, "status: %s",Connection->status);
    return 0;
}


static int on_url(http_parser *parser, const char *at, size_t len) {
    Connection *Connection = (struct Connection*)parser->data;
    
    Connection->path = new char[len+1]();
   
    
    strncpy(Connection->path,at,len);
    // printf("%s",Connection->path);
     otbrLog(OTBR_LOG_ERR, "path: %s",Connection->path);

    return 0;
}

static int on_header_field(http_parser *parser, const char *at, size_t len) {
    Connection *Connection = (struct Connection*)parser->data;
    Connection->header_f = new char[len+1]();
   
    
    strncpy(Connection->header_f,at,len);
    otbrLog(OTBR_LOG_ERR, "header_f: %s",Connection->header_f);
    // if (Connection->header_state != 1) {
    //     header = malloc(sizeof(struct header));
    //     header->name = NULL;
    //     header->value = NULL;
    //     header->next = Connection->headers;
    //     Connection->headers = header;
    // } else {
    //     header = Connection->headers;
    // }

    // header->name = sdscatlen(header->name == NULL ? sdsempty() : header->name, at, len);
    // Connection->header_state = 1;

    return 0;
}

static int on_header_value(http_parser *parser, const char *at, size_t len) {
    Connection *connection = (struct Connection*)parser->data;
    connection->header_v = new char[len+1]();
    
    strncpy(connection->header_v,at,len);
    otbrLog(OTBR_LOG_ERR, "header_v: %s",connection->header_v);
    return 0;
}
static int on_body(http_parser *parser, const char *at, size_t len) {
    Connection *connection =(struct Connection*) parser->data;
    connection->body = new char[len+1]();
    strncpy(connection->body,at,len-1);
    
    otbrLog(OTBR_LOG_ERR, "body : %s",connection->body);
    return 0;
}

static int on_headers_complete(http_parser *parser) {
    Connection *connection =(struct Connection*) parser->data;
    connection->content_length = parser->content_length;
    connection->method = parser->method;
    return 0;
}

static int on_message_complete(http_parser *parser) {
    Connection *connection =(struct Connection*) parser->data;
    connection->complete = 0;
    return 0;
}

static int on_chunk_header(http_parser *parser) {
    Connection *connection =(struct Connection*) parser->data;
    connection->complete = 0;
    return 0;
}

static int on_chunk_complete(http_parser *parser) {
    Connection *connection =(struct Connection*) parser->data;
    connection->complete = 0;
    return 0;
}

static int setNonBlocking(int fd)
{
	int oldflags;

	oldflags = fcntl(fd, F_GETFL);

	if (fcntl(fd, F_SETFL, oldflags | O_NONBLOCK) == -1) {
		// SOCKETIO_SYSERROR("fcntl");
		return -1;
	}

	return 0;
}

static http_parser_settings mSettings {
        .on_message_begin = on_message_begin,
        .on_url = on_url,
        .on_status = on_status,
        .on_header_field = on_header_field,
        .on_header_value = on_header_value,
        .on_headers_complete = on_headers_complete,
        .on_body = on_body,
        .on_message_complete = on_message_complete,
        .on_chunk_header = on_chunk_header,
        .on_chunk_complete = on_chunk_complete
};

RestWebServer::RestWebServer(otbr::Ncp::ControllerOpenThread *aNcp):
    mNcp(aNcp),
    mAddress(new sockaddr_in())
    
{
   
   

  
}

void RestWebServer::init(){
    //otbrError   error = OTBR_ERROR_NONE;
    mThreadHelper = mNcp->GetThreadHelper();
    mInstance = mThreadHelper->GetInstance();
    
    handlerInit();
   

//     mParser = (http_parser*)malloc(sizeof(http_parser));

    
    //to do ? set interface name
    //get interface 

    
    mAddress->sin_family = AF_INET;
    mAddress->sin_addr.s_addr = INADDR_ANY;
    mAddress->sin_port = htons(80);

    mListenFd = socket(AF_INET, SOCK_STREAM, 0);
    otbrLog(OTBR_LOG_ERR, "socket success" );
    
    // struct timeval recvTimeout;
    // recvTimeout.tv_sec = 1;
    // recvTimeout.tv_usec = 0;
    //setsockopt(mListenFd,SOL_SOCKET,SO_RCVTIMEO, (char*) &recvTimeout, sizeof(recvTimeout));

    bind(mListenFd, (struct sockaddr *) mAddress, sizeof(sockaddr)) ;
    otbrLog(OTBR_LOG_ERR, "bind success" );
    

    //need to set backlog
    listen(mListenFd, 5);
    otbrLog(OTBR_LOG_ERR, "listen success" );
    
    setNonBlocking(mListenFd);
    // int flags = fcntl(master_socket, F_GETFL);
    // fcntl(master_socket, F_SETFL,flags| O_NONBLOCK);


}


void RestWebServer::handlerInit(){
        mHandlerMap.emplace("/diagnostics",&diagnosticRequestHandler);
        mHandlerMap.emplace("/node/network-name",&getJsonNetworkName);
        mHandlerMap.emplace("/node/rloc16",&getJsonRloc16);
        mHandlerMap.emplace("/node/leader-data",&getJsonLeaderData);
        mHandlerMap.emplace("/node/num-of-route",&getJsonNumOfRoute);
        mHandlerMap.emplace("/node/ext-panid",&getJsonExtendedPanId);
        mHandlerMap.emplace("/node/rloc",&getJsonRloc);
        


        // mHandlerMap.emplace("/node/network-name",&networkNameRequestHandler);

}


 void RestWebServer::UpdateFdSet(fd_set &aReadFdSet, int &aMaxFd,timeval &aTimeout){

        
        struct timeval timeout        = {60*60*24,0};
        otThreadSetReceiveDiagnosticGetCallback(mInstance, &RestWebServer::diagnosticResponseHandler, this);
        
        FD_SET(mListenFd, &aReadFdSet);
        if (aMaxFd < mListenFd){
        aMaxFd =  mListenFd;
        }
        
        for ( auto it = mConnectionSet.begin(); it != mConnectionSet.end(); ++it ){
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - it->second->start).count();
        
        
            if (duration <= mTimeout){

                timeout.tv_sec = 0; 
                if (timeout.tv_usec == 0){
                    timeout.tv_usec = std::max(0, mTimeout - (int)duration );
                }
                else{
                   timeout.tv_usec = std::min(int(timeout.tv_usec ),std::max(0, mTimeout - (int)duration ));
                }
               
                FD_SET(it->first, &aReadFdSet);
                if (aMaxFd < it->first){
                    aMaxFd =  it->first;
                }

            }
            else{
                timeout.tv_sec = 0; 
                timeout.tv_usec = 0; 
            }
        
        }
        
        if (timercmp(&timeout, &aTimeout, <))
        {
            aTimeout = timeout;
        }

        

}




void RestWebServer::Process( fd_set &aReadFdSet){
    socklen_t addrlen  = sizeof(sockaddr);;
    int fd;
    auto err = errno;
    
    for ( auto it = mConnectionSet.begin(); it != mConnectionSet.end(); ++it ){
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - it->second->start).count();
        if( duration > mTimeout ){
            serveRequest(it->first);
        }
        
        else{
        if (FD_ISSET(it->first, &aReadFdSet))
            // serveRequest(it->first);
            nonBlockRead(it->first);
        }

        

    }
    auto eraseIt = mConnectionSet.begin();
    for ( eraseIt = mConnectionSet.begin(); eraseIt != mConnectionSet.end();  ){
         if (eraseIt->second->complete == 1) {
            delete(eraseIt->second);
            eraseIt = mConnectionSet.erase(eraseIt);
         }
         else eraseIt++;
    }
    // mConnectionSet.erase(fd);
    
    if (FD_ISSET(mListenFd, &aReadFdSet)){
        
        while (true){
            fd = accept(mListenFd, (struct sockaddr *) mAddress, &addrlen);
            err = errno;
            if (fd < 0) break;
            
            if(mConnectionSet.size() < (unsigned)mMaxServeNum){ 
                //set up new connection
                setNonBlocking(fd);
                mConnectionSet[fd] = new Connection(std::chrono::steady_clock::now());
                mConnectionSet[fd]->fd = fd;
                
                otbrLog(OTBR_LOG_ERR, "add new fd: %d",fd );
            }
            else
            { 

                //still need response  todo
                otbrLog(OTBR_LOG_ERR, "server is busy");

            }
        }

        if (err == EWOULDBLOCK){
            otbrLog(OTBR_LOG_ERR, "Having accept all connections" );
        }
        else{
            otbrLog(OTBR_LOG_ERR, "Accept error, should havndle it" );
        }
    }
    
}




cJSON* RestWebServer::getJSON(int fd){
    cJSON * ret = NULL;
    Connection* mConnection= mConnectionSet[fd];
    std::string url = mConnection->path;
    auto Handler = mHandlerMap.find(url);
    if (Handler == mHandlerMap.end()){
        otbrLog(OTBR_LOG_ERR, "no matching Handler");
        mConnection->isError = 1;
    }
    else{
        ret = (*Handler->second)( mConnection,mInstance);
    }
    return ret;
    

    
    
}



void RestWebServer::parseUri(int fd){
    Connection* mConnection= mConnectionSet[fd];
    char * queryStart = NULL;
    int pc=0;
    char *tok;
    char *otok;
    char* querystring;
    char* p;
    for (queryStart = mConnection->path ; queryStart  < mConnection->path + strlen(mConnection->path); queryStart++)
        if (*queryStart == '?'){
            
            break;
        }
    
    if (queryStart != mConnection->path + strlen(mConnection->path)){
        p = new char[strlen(mConnection->path)+1]();
        strncpy(p,mConnection->path,queryStart-mConnection->path+1);
        delete(mConnection->path);
        mConnection->path = p;
        queryStart++;
        querystring = new char[strlen(mConnection->path)+1]();
        strcpy(querystring, queryStart); 
        for(tok=strtok(querystring,"&");tok!=NULL;tok=strtok(tok,"&")) {
            pc++;
            otok=tok+strlen(tok)+1;
            tok=strtok(tok,"=");
            fprintf(stderr,"param%d: %s ",pc,tok);
            tok=strtok(NULL,"=");
            fprintf(stderr,"value%d: %s\n",pc,tok);
            tok=otok;
        };
       
    }
    
}

void RestWebServer::nonBlockRead(int fd){
    Connection* mConnection= mConnectionSet[fd];
    if (!mConnection->buf){
        otbrLog(OTBR_LOG_ERR, "first process fd  %d",fd );
        mConnection->buf = new char[mConnection->remain+1]();
        mConnection->b = mConnection->buf;
        
    }
    int received = 0;
    int count = 0;
    while ((received = read(fd,mConnection->b,mConnection->remain - mConnection->len)) > 0){
        mConnection->len += received;
        mConnection->remain-=received;
        
         
        otbrLog(OTBR_LOG_ERR, "normal read length %d",received );
        otbrLog(OTBR_LOG_ERR, "normal read length %s",mConnection->b );
        mConnection->b += received;
    }
    if (received == -1 && (errno == EAGAIN || errno == EWOULDBLOCK  ) ){
        otbrLog(OTBR_LOG_ERR, "normal read exit, no data available now");

    }
    
    else{
        if (received == 0)
        // otbrLog(OTBR_LOG_ERR, "");
        count++;

        else
        otbrLog(OTBR_LOG_ERR, "error occur, should handle( to do)");
    }



}

void RestWebServer::FreeConnection(int fd){
        Connection* connection = mConnectionSet[fd];
        otbrLog(OTBR_LOG_ERR, "start free");
        
        connection->complete = 1;
        otbrLog(OTBR_LOG_ERR, "start free1" );
        close(fd);
        otbrLog(OTBR_LOG_ERR, "start free2" );

    }


void RestWebServer::serveRequest(int fd){

    cJSON* data = NULL;
    Connection* mConnection= mConnectionSet[fd];
    mConnection->mParser = new http_parser();
    mConnection->mParser->data = mConnection;
    http_parser_init(mConnection->mParser, HTTP_REQUEST);
    if (mConnection->len == 0 ){

        otbrLog(OTBR_LOG_ERR, "nothing is read, exit fd:%d", fd );
        mConnection->isError = 1;
            
        
    }
    else{

        otbrLog(OTBR_LOG_ERR, "start processing fd:%d",fd );
        //parse
        http_parser_execute(mConnection->mParser, &mSettings, mConnection->buf , strlen(mConnection->buf));
        
        //call handler
        data = getJSON(fd);
        
       
    }
    if (mConnection->hasCallback == 0){
    otbrLog(OTBR_LOG_ERR, "start send response");
    sentResponse(data,mConnection);
    FreeConnection(fd);
    }
    else{
        otbrLog(OTBR_LOG_ERR, "wait for callback");
    }
        
}

void RestWebServer::sentResponse(cJSON* data, Connection* connection){
    
    otbrLog(OTBR_LOG_ERR, "start send");
    char * out;
    char * p ;
    
    
    
    cJSON *root,*cars;
    root = cJSON_CreateObject();
    cars = cJSON_CreateArray();
    cJSON_AddItemToObject(root,"Error",cars);
    out = cJSON_Print(root);
    // otbrLog(OTBR_LOG_ERR, "test json data %s",p);
    if ( connection->isError == 1 ){
    p = out;  
    }
    else{
    p = cJSON_Print(data);
    //need more 
    }
    char h[] = "HTTP/1.1 200 OK\r\n";
    char h1[] = "Content-Type: application/json\r\n";
    
        
   
    write(connection->fd, h, strlen(h));
    write(connection->fd, h1, strlen(h1));
    write(connection->fd, "\r\n", 2);
    write(connection->fd,p, strlen(p));
    // write(connection->fd,p, strlen(p));
    otbrLog(OTBR_LOG_ERR, "finish send");
    free(p);

    // cJSON_Delete(data);


     


}



uint16_t RestWebServer::Swap16(uint16_t v)


{
    return (((v & 0x00ffU) << 8) & 0xff00) | (((v & 0xff00U) >> 8) & 0x00ff);
}

cJSON* RestWebServer::getJsonNetworkName(Connection* connection,otInstance * mInstance){
    connection->hasCallback = 0;
    const char *networkName = otThreadGetNetworkName(mInstance);
    char *p = new char[20]();
    sprintf(p,"%s",static_cast<const char *>(networkName));
    cJSON* ret = cJSON_CreateString(p);
    // delete(p);
    
    return ret;
}

cJSON*  RestWebServer::getJsonLeaderData(Connection* connection,otInstance * mInstance){
    connection->hasCallback = 0;
    otLeaderData leaderData;
    otThreadGetLeaderData(mInstance, &leaderData);
    cJSON* ret = CreateJsonLeaderData(leaderData);
    return ret;

}

cJSON*  RestWebServer::getJsonNumOfRoute( Connection* connection, otInstance * mInstance){
    connection->hasCallback = 0;
     int count = 0;
     uint8_t maxRouterId;
     otRouterInfo routerInfo;
     maxRouterId = otThreadGetMaxRouterId(mInstance);
     char *p = new char[6]();
     
    // cJSON * cTmp = cJSON_CreateString(tmp);
    

        for (uint8_t i = 0; i <= maxRouterId; i++)
        {
            if (otThreadGetRouterInfo(mInstance, i, &routerInfo) != OT_ERROR_NONE)
            {
                continue;
            }
            count++;
        }
    sprintf(p,"%d",count);
    cJSON * ret = cJSON_CreateString(p);
    // delete(p);
    return ret;

}
cJSON*  RestWebServer::getJsonRloc16(Connection* connection, otInstance * mInstance){
    connection->hasCallback = 0;
    uint16_t rloc16 = otThreadGetRloc16(mInstance);
    char *p = new char[7]();
    sprintf(p,"0x%04x",rloc16);
    cJSON * ret = cJSON_CreateString(p);
    // delete(p);
    return ret;
}

cJSON*  RestWebServer::getJsonExtendedPanId(Connection* connection, otInstance * mInstance){
    connection->hasCallback = 0;
    const uint8_t *extPanId = reinterpret_cast<const uint8_t *>(otThreadGetExtendedPanId(mInstance));
    char *p = new char[17]();
    char *q = p;
    // sprintf(p,"%02x%02x%02x%02x%02x%02x%02x%02x",extPanId[0],extPanId[1],extPanId[2],
    // extPanId[3],extPanId[4],extPanId[5],extPanId[6],extPanId[7]);
    for (int i = 0; i <  OT_EXT_PAN_ID_SIZE; i++)
    {
       snprintf(q,3,"%02x",extPanId[i]);
       q+=2;
    }
    cJSON * ret = cJSON_CreateString(p);
    // delete(p);
    return ret;
}

cJSON*  RestWebServer::getJsonRloc(Connection* connection, otInstance * mInstance){
    connection->hasCallback = 0;
    struct otIp6Address rloc16address =  *otThreadGetRloc(mInstance);
    char *p = new char[65]();
    sprintf(p,"%x:%x:%x:%x:%x:%x:%x:%x",Swap16(rloc16address.mFields.m16[0]), Swap16(rloc16address.mFields.m16[1]),
        Swap16(rloc16address.mFields.m16[2]), Swap16(rloc16address.mFields.m16[3]), Swap16(rloc16address.mFields.m16[4]),
        Swap16(rloc16address.mFields.m16[5]), Swap16(rloc16address.mFields.m16[6]), Swap16(rloc16address.mFields.m16[7]));
    cJSON * ret = cJSON_CreateString(p);
    // delete(p);
    return ret;

}




cJSON* RestWebServer::diagnosticRequestHandler(Connection* connection , otInstance *  mInstance){
    connection->hasCallback = 1;
    struct otIp6Address rloc16address =  *otThreadGetRloc(mInstance);
    cJSON* ret = NULL;

    char *p = new char[65]();
    sprintf(p,"%x:%x:%x:%x:%x:%x:%x:%x",Swap16(rloc16address.mFields.m16[0]), Swap16(rloc16address.mFields.m16[1]),
        Swap16(rloc16address.mFields.m16[2]), Swap16(rloc16address.mFields.m16[3]), Swap16(rloc16address.mFields.m16[4]),
        Swap16(rloc16address.mFields.m16[5]), Swap16(rloc16address.mFields.m16[6]), Swap16(rloc16address.mFields.m16[7]));
    uint8_t             tlvTypes[OT_NETWORK_DIAGNOSTIC_TYPELIST_MAX_ENTRIES];
    uint8_t             count     = 0;
    
    for(long t = 1; t <= 8; t++){
        tlvTypes[count++] = static_cast<uint8_t>(t);
    }
    
    otThreadSendDiagnosticGet(mInstance, &rloc16address, tlvTypes, count);
    
    return ret;

         
    

}





cJSON* RestWebServer::CreateJsonConnectivity(const otNetworkDiagConnectivity &aConnectivity){

    cJSON *ret = cJSON_CreateObject();

    char* tmp = new char[11]();
    
   

    sprintf(tmp,"%d",aConnectivity.mParentPriority);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "ParentPriority", cJSON_CreateString(tmp));

    sprintf(tmp,"%u",aConnectivity.mLinkQuality3);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "LinkQuality3", cJSON_CreateString(tmp));

    sprintf(tmp,"%u",aConnectivity.mLinkQuality2);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "LinkQuality2", cJSON_CreateString(tmp));

    sprintf(tmp,"%u",aConnectivity.mLinkQuality1);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "LinkQuality1", cJSON_CreateString(tmp));

    sprintf(tmp,"%u",aConnectivity.mLeaderCost);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "LeaderCost", cJSON_CreateString(tmp));

    sprintf(tmp,"%u",aConnectivity.mIdSequence);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "IdSequence", cJSON_CreateString(tmp));

    sprintf(tmp,"%u",aConnectivity.mActiveRouters);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "ActiveRouters", cJSON_CreateString(tmp));

    sprintf(tmp,"%u",aConnectivity.mSedBufferSize);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "SedBufferSize", cJSON_CreateString(tmp));

    sprintf(tmp,"%u",aConnectivity.mSedDatagramCount);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "SedDatagramCount", cJSON_CreateString(tmp));
    
    return ret;
    

    
    
   
}
 cJSON* RestWebServer::CreateJsonMode(const otLinkModeConfig &aMode){

    cJSON *ret = cJSON_CreateObject();

    char* tmp = new char[2]();
    
    sprintf(tmp,"%d",aMode.mRxOnWhenIdle);
    // cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "RxOnWhenIdle", cJSON_CreateString(tmp));
    
   
    sprintf(tmp,"%d",aMode.mSecureDataRequests);
    //  cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "SecureDataRequests", cJSON_CreateString(tmp));


    sprintf(tmp,"%d",aMode.mDeviceType);
    // cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "DeviceType", cJSON_CreateString(tmp));
   
    sprintf(tmp,"%d",aMode.mNetworkData);
    // cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "NetworkData", cJSON_CreateString(tmp));

    return ret;
    

    
    
   
}

 cJSON* RestWebServer::CreateJsonRoute(const otNetworkDiagRoute &aRoute){

    cJSON *ret = cJSON_CreateObject();

    char* tmp = new char[11]();
    
    sprintf(tmp,"%d",aRoute.mIdSequence);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(ret, "IdSequence", cJSON_CreateString(tmp));

    cJSON* RouteData = cJSON_CreateArray();
    for (uint16_t i = 0; i < aRoute.mRouteCount; ++i)
    {   
        cJSON* RouteDatatmp = CreateJsonRouteData(aRoute.mRouteData[i]);
        
        cJSON_AddItemToArray(RouteData,RouteDatatmp);
    }
    
    cJSON_AddItemToObject(ret, "RouteData", RouteData );
    
   
    

    return ret;
    

    
    
   
}
 cJSON* RestWebServer::CreateJsonRouteData(const otNetworkDiagRouteData &aRouteData){
    char* tmp = new char[5]();
    cJSON* RouteData = cJSON_CreateObject();

    sprintf(tmp,"0x%02x",aRouteData.mRouterId);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(RouteData, "RouteId", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aRouteData.mLinkQualityOut);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(RouteData, "LinkQualityOut", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aRouteData.mLinkQualityIn);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(RouteData, "LinkQualityIn", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aRouteData.mRouteCost);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(RouteData, "RouteCost", cJSON_CreateString(tmp));


    return RouteData;






}



cJSON* RestWebServer::CreateJsonLeaderData(const otLeaderData &aLeaderData){
    char* tmp = new char[9]();
    cJSON* LeaderData = cJSON_CreateObject();

    sprintf(tmp,"0x%08x",aLeaderData.mPartitionId);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(LeaderData, "PartitionId", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aLeaderData.mWeighting);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(LeaderData, "Weighting", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aLeaderData.mDataVersion);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(LeaderData, "DataVersion", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aLeaderData.mStableDataVersion);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(LeaderData, "StableDataVersion", cJSON_CreateString(tmp));

    sprintf(tmp,"0x%02x",aLeaderData.mLeaderRouterId);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(LeaderData, "LeaderRouterId", cJSON_CreateString(tmp));

    return LeaderData;


}
cJSON* RestWebServer::CreateJsonIp6Address(const otIp6Address &aAddress) {
        char * tmp = new char[65]();
        // sprintf(tmp,"%x:%x:%x:%x:%x:%x:%x:%x",HostSwap16(aAddress.mFields.m16[0]), HostSwap16(aAddress.mFields.m16[1]),
        // HostSwap16(aAddress.mFields.m16[2]), HostSwap16(aAddress.mFields.m16[3]), HostSwap16(aAddress.mFields.m16[4]),
        // HostSwap16(aAddress.mFields.m16[5]), HostSwap16(aAddress.mFields.m16[6]), HostSwap16(aAddress.mFields.m16[7]));
        sprintf(tmp,"%x:%x:%x:%x:%x:%x:%x:%x",aAddress.mFields.m16[0], aAddress.mFields.m16[1],
        aAddress.mFields.m16[2], aAddress.mFields.m16[3], aAddress.mFields.m16[4],
        aAddress.mFields.m16[5], aAddress.mFields.m16[6], aAddress.mFields.m16[7]);
        
        cJSON* ret = cJSON_CreateString(tmp);
        return ret;

}

cJSON* RestWebServer::CreateJsonMacCounters(const otNetworkDiagMacCounters &aMacCounters){
    char* tmp = new char[9]();
    cJSON* cMacCounters = cJSON_CreateObject();

    sprintf(tmp,"%d",aMacCounters.mIfInUnknownProtos);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfInUnknownProtos", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aMacCounters.mIfInErrors);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfInErrors", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aMacCounters.mIfOutErrors);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfOutErrors", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aMacCounters.mIfInUcastPkts);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfInUcastPkts", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aMacCounters.mIfInBroadcastPkts);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfInBroadcastPkts", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aMacCounters.mIfInDiscards);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfInDiscards", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aMacCounters.mIfOutUcastPkts);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfOutUcastPkts", cJSON_CreateString(tmp));
    
    sprintf(tmp,"%d",aMacCounters.mIfOutBroadcastPkts);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfOutBroadcastPkts", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aMacCounters.mIfOutDiscards);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cMacCounters, "IfOutDiscards", cJSON_CreateString(tmp));
    return cMacCounters;
   
}



 cJSON* RestWebServer::CreateJsonChildTableEntry(const otNetworkDiagChildEntry &aChildEntry){

    char* tmp = new char[7]();
    cJSON* cChildTableEntry = cJSON_CreateObject();

    sprintf(tmp,"0x%04x",aChildEntry.mChildId);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cChildTableEntry, "ChildId", cJSON_CreateString(tmp));

    sprintf(tmp,"%d",aChildEntry.mTimeout);
    // cJSON * cTmp = cJSON_CreateString(tmp);
    cJSON_AddItemToObject(cChildTableEntry, "Timeout", cJSON_CreateString(tmp));

    cJSON* cMode = CreateJsonMode(aChildEntry.mMode);
    cJSON_AddItemToObject(cChildTableEntry, "Mode", cMode);
    

    return cChildTableEntry;


}

void RestWebServer::diagnosticResponseHandler(otMessage *aMessage, const otMessageInfo *aMessageInfo, void *aContext){
    otbrLog(OTBR_LOG_ERR, "ddddddd" );
    static_cast<RestWebServer *>(aContext)->diagnosticResponseHandler(
        aMessage, *static_cast<const otMessageInfo *>(aMessageInfo));
}


void RestWebServer::diagnosticResponseHandler(otMessage *aMessage, const otMessageInfo &aMessageInfo ){
    otbrLog(OTBR_LOG_ERR, "uuuuuuu" );
    uint8_t               buf[16];
    uint16_t              bytesToPrint;
    uint16_t              bytesPrinted = 0;
    uint16_t              length       = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    otNetworkDiagTlv      diagTlv;
    otNetworkDiagIterator iterator = OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT;
    otError               error    = OT_ERROR_NONE;
    cJSON *ret = cJSON_CreateObject();

    
    auto sockRloc16 = aMessageInfo.mPeerAddr.mFields.m16[7];
    otbrLog(OTBR_LOG_ERR, "dddddd%02x", sockRloc16);
    
    while (length > 0){
        bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
        otMessageRead(aMessage, otMessageGetOffset(aMessage) + bytesPrinted, buf, bytesToPrint);
        // OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));


        length -= bytesToPrint;
        bytesPrinted += bytesToPrint;

    }
    
    while ((error = otThreadGetNextDiagnosticTlv(aMessage, &iterator, &diagTlv)) == OT_ERROR_NONE)
    {
        
        switch (diagTlv.mType)
        {
        case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:{
            
            cJSON * extAddress = cJSON_CreateString(reinterpret_cast<const char*>(diagTlv.mData.mExtAddress.m8));
            cJSON_AddItemToObject(ret, "Ext Address", extAddress);
        }


            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
        {
            char* rloc = new char[7]();
            sprintf(rloc,"0x%04x",diagTlv.mData.mAddr16);
            cJSON * cRloc16 = cJSON_CreateString(rloc);
            cJSON_AddItemToObject(ret, "Rloc16", cRloc16);
            otbrLog(OTBR_LOG_ERR, "rloc16 %s",rloc );
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MODE:{
            cJSON * cMode = CreateJsonMode(diagTlv.mData.mMode);
            cJSON_AddItemToObject(ret, "Mode", cMode);
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT:{
            char* timeout = new char[11]();
            sprintf(timeout,"%d",diagTlv.mData.mTimeout);
            cJSON * cTimeout = cJSON_CreateString(timeout);
            cJSON_AddItemToObject(ret, "Timeout", cTimeout);
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:{
            cJSON * cConnectivity = CreateJsonConnectivity(diagTlv.mData.mConnectivity);
            cJSON_AddItemToObject(ret, "Connectivity", cConnectivity);
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:{

            cJSON * cRoute = CreateJsonRoute(diagTlv.mData.mRoute);
            cJSON_AddItemToObject(ret, "Route", cRoute);
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:{

            cJSON * cLeaderData = CreateJsonLeaderData(diagTlv.mData.mLeaderData);
            cJSON_AddItemToObject(ret, "Leader Data", cLeaderData);
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:{

            char* networkData = new char[diagTlv.mData.mNetworkData.mCount+1]();
            snprintf(networkData,diagTlv.mData.mNetworkData.mCount+1,"%s",reinterpret_cast<const char*>(diagTlv.mData.mNetworkData.m8));
            cJSON * cNetworkData = cJSON_CreateString(networkData);
            cJSON_AddItemToObject(ret, "Network Data", cNetworkData );
        }
           
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:{
            
            cJSON* cIP6List = cJSON_CreateArray();
            for (uint16_t i = 0; i < diagTlv.mData.mIp6AddrList.mCount; ++i){
                
                cJSON * cIP6Address = CreateJsonIp6Address(diagTlv.mData.mIp6AddrList.mList[i]);
                
                cJSON_AddItemToArray(cIP6List,cIP6Address);
            }
            cJSON_AddItemToObject(ret, "IP6 Address List", cIP6List );
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:{
            cJSON * cMacCounters = CreateJsonMacCounters(diagTlv.mData.mMacCounters);
            cJSON_AddItemToObject(ret, "MAC Counters", cMacCounters);
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:{
            char * batteryLevel = new char[9]();
            sprintf(batteryLevel ,"%d",diagTlv.mData.mBatteryLevel);
            cJSON * cBatteryLevel = cJSON_CreateString(batteryLevel);
            cJSON_AddItemToObject(ret, "Battery Level", cBatteryLevel );
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE:{
            char * supplyVoltage = new char[9]();
            sprintf(supplyVoltage ,"%d",diagTlv.mData.mSupplyVoltage);
            cJSON * cSupplyVoltage = cJSON_CreateString(supplyVoltage);
            cJSON_AddItemToObject(ret, "Supply Voltage", cSupplyVoltage );
        }
           
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:{

            cJSON* cChildTable = cJSON_CreateArray();
            
            for (uint16_t i = 0; i < diagTlv.mData.mChildTable.mCount; ++i)
            {
                cJSON* cChildTableEntry = CreateJsonChildTableEntry(diagTlv.mData.mChildTable.mTable[i]);
                cJSON_AddItemToArray(cChildTable,cChildTableEntry);
               
            }
            cJSON_AddItemToObject(ret, "Child Table", cChildTable );
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES:{

            char* channelPages = new char[diagTlv.mData.mChannelPages.mCount+1]();
            snprintf(channelPages,diagTlv.mData.mChannelPages.mCount+1,"%s",reinterpret_cast<const char*>(diagTlv.mData.mChannelPages.m8));
            cJSON * cChannelPages = cJSON_CreateString(channelPages);
            cJSON_AddItemToObject(ret, "Channel Pages", cChannelPages );
        }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT:{

            char * childTimeout = new char[9]();
            sprintf(childTimeout ,"%d",diagTlv.mData.mMaxChildTimeout);
            cJSON * cChildTimeout = cJSON_CreateString(childTimeout);
            cJSON_AddItemToObject(ret, "Max Child Timeout", cChildTimeout );
        }
            
            break;
        }
    }
}




}
}