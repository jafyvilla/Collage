
/* Copyright (c) 2005, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#include "server.h"

#include "connection.h"
#include "global.h"

#include <eq/base/log.h>

using namespace eqNet;
using namespace eqNet::priv;
using namespace std;

int Server::run( Connection* connection )
{
    Server* server = new Server( connection );
    return server->_run();
}

Server::Server( Connection* connection )
        : Node(NODE_ID_SERVER)
{
    _connections.push_back(connection);
}

//----------------------------------------------------------------------
// initialization
//----------------------------------------------------------------------

void Server::_init()
{
}

//----------------------------------------------------------------------
// main loop
//----------------------------------------------------------------------
int Server::_run()
{
    while( true )
    {
        short event;
        Connection *connection = Connection::select( _connections, -1, event );

        switch( event )
        {
            case 0:
                WARN << "Got timeout during connection selection?" << endl;
                break;

            case POLLERR:
                WARN << "Got error during connection selection" << endl;
                break;

            case POLLIN:
            case POLLPRI: // data is ready for reading
                _handleRequest( connection );
                break;

            case POLLHUP: // disconnect happened
                WARN << "Unhandled: Connection disconnect" << endl;
                break;

            case POLLNVAL: // disconnected connection
                WARN << "Unhandled: Disconnected connection" << endl;
                break;
        }
        break;
    }

    return EXIT_SUCCESS;
}

void Server::_handleRequest( Connection *connection )
{
    switch( connection->getState() )
    {
        case Connection::STATE_CONNECTED:
            break;
            
        case Connection::STATE_LISTENING:
            Connection *newConn = connection->accept();
            _connections.push_back( newConn );
            break;
    }
}


//----------------------------------------------------------------------
// proxy initialisation
//----------------------------------------------------------------------
Server* Server::connect( Connection* connection )
{
}
