
/* Copyright (c) 2006-2009, Stefan Eilemann <eile@equalizergraphics.com> 
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *  
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// Tests network throughput
// Usage: see 'netPerf -h'

#include <pthread.h> // must come first!

#include <test.h>
#include <eq/base/scopedMutex.h>
#include <eq/base/monitor.h>
#include <eq/base/rng.h>
#include <eq/base/sleep.h>
#include <eq/net/connection.h>
#include <eq/net/connectionDescription.h>
#include <eq/net/connectionSet.h>
#include <eq/net/connectionType.h>
#include <eq/net/init.h>

#ifndef MIN
#  define MIN EQ_MIN
#endif
#include <tclap/CmdLine.h>

#include <iostream>

using namespace eq::net;
using namespace std;

namespace
{
ConnectionSet    _connectionSet;
eq::base::mtLong _nClients;
eq::base::Lock   _mutexPrint;
class Receiver : public eq::base::Thread
{
public:
    Receiver( const size_t packetSize, ConnectionPtr connection )
            : _connection( connection )
            , _mBytesSec( packetSize / 1024.f / 1024.f * 1000.f )
            , _nSamples( 0 )
        { 
            _buffer.resize( packetSize );
            connection->recvNB( _buffer.getData(), _buffer.getSize() );
        }

    virtual ~Receiver() {}

    bool readPacket()
        {
            if( !_connection->recvSync( 0, 0 ))
                return false;
            uint32_t num = _buffer.getData()[0];
            _connection->recvNB( _buffer.getData(), _buffer.getSize() );
            const float time = _clock.getTimef();
            ++_nSamples;

            const size_t probe = 1 + (_rng.get< size_t >() % (_buffer.getSize( )-1));
            TESTINFO( _buffer[probe] == static_cast< uint8_t >( probe ),
                      (int)_buffer[probe] << " != " << (probe&0xff) );

            if( time < 1000.f )
                return true;

            _clock.reset();
            eq::net::ConnectionDescriptionPtr desc = 
                _connection->getDescription();
            const eq::base::ScopedMutex mutex( _mutexPrint );
            cerr << num << " Recv perf: " << _mBytesSec / time * _nSamples 
                 << "MB/s (" << _nSamples / time * 1000.f  << "pps) from "
                 << desc.get() << endl;
            _nSamples = 0;
            return true;
        }

    void executeReceive()
        {
            TEST( _hasConnection == false );
            _hasConnection = true;
        }

    void stop()
        {
            TEST( _hasConnection == false );
            _connection = 0;
            _hasConnection = true;
        }

    virtual void* run()
        {
            _hasConnection.waitEQ( true );
            while( _connection.isValid( ))
            {
                if( !readPacket( )) // dead connection
                {
                    cerr << --_nClients << " clients" << endl;
                    _connectionSet.interrupt();
                    _connection = 0;
                    return EXIT_SUCCESS;
                }

                _hasConnection = false;
                _connectionSet.addConnection( _connection );
                _hasConnection.waitEQ( true );
            }
            return EXIT_SUCCESS;
        }

private:
    eq::base::Clock _clock;
    eq::base::RNG _rng;

    eq::base::Bufferb _buffer;
    eq::base::Monitor< bool > _hasConnection;
    ConnectionPtr             _connection;
    const float _mBytesSec;
    size_t      _nSamples;
    size_t      _packetSize;
};



class Selector : public eq::base::Thread
{
public:
    Selector( ConnectionPtr connection, const size_t packetSize,
              const bool useThreads )
            : _connection( connection )
            , _packetSize( packetSize )
            , _useThreads( useThreads )
        {
        }

    virtual bool init()
        {
            TEST( _connection->listen( ));
            _connection->acceptNB();

            _connectionSet.addConnection( _connection );

            // Get first client
            const ConnectionSet::Event event = _connectionSet.select();
            TEST( event == ConnectionSet::EVENT_CONNECT );

            ConnectionPtr resultConn = _connectionSet.getConnection();
            ConnectionPtr newConn    = resultConn->acceptSync();
            resultConn->acceptNB();
        
            TEST( resultConn == _connection );
            TEST( newConn.isValid( ));

            _receivers.push_back( RecvConn( new Receiver( _packetSize, newConn),
                                            newConn ));
            if( _useThreads )
                _receivers.back().first->start();

            _connectionSet.addConnection( newConn );
            // Until all client have disconnected...
            _nClients = 1;
            return true;
        }
    virtual void* run()
        {
            ConnectionPtr resultConn;
            ConnectionPtr newConn;
            while( _nClients > 0 )
            {
                switch( _connectionSet.select( )) // ...get next request
                {
                    case ConnectionSet::EVENT_CONNECT: // new client
                    
                        resultConn = _connectionSet.getConnection();
                        newConn = resultConn->acceptSync();
                        resultConn->acceptNB();

                        TEST( newConn.isValid( ));

                        _receivers.push_back( 
                            RecvConn( new Receiver( _packetSize, newConn ),
                                      newConn ));
                        if( _useThreads )
                            _receivers.back().first->start();

                        _connectionSet.addConnection( newConn );
                        cerr << ++_nClients << " clients" << endl;
                        break;

                    case ConnectionSet::EVENT_DATA:  // new data
                    {
                        resultConn = _connectionSet.getConnection();

                        Receiver* receiver = 0;
                        vector< RecvConn >::iterator i;
                        for( i = _receivers.begin(); i != _receivers.end(); ++i)
                        {
                            const RecvConn& candidate = *i;
                            if( candidate.second == resultConn )
                            {
                                receiver = candidate.first;
                                break;
                            }
                        }
                        TEST( receiver );

                        if( _useThreads )
                        {
                            _connectionSet.removeConnection( resultConn );
                            receiver->executeReceive();
                        }
                        else if( !receiver->readPacket())
                        {
                            // Connection dead?
                            _connectionSet.removeConnection( resultConn );
                            delete receiver;
                            _receivers.erase( i );
                            cerr << --_nClients << " clients" << endl;
                        }
                        break;
                    }
                    case ConnectionSet::EVENT_DISCONNECT:
                    case ConnectionSet::EVENT_INVALID_HANDLE:  // client done
                        resultConn = _connectionSet.getConnection();
                        _connectionSet.removeConnection( resultConn );

                        for( vector< RecvConn >::iterator i =_receivers.begin();
                             i != _receivers.end(); ++i )
                        {
                            const RecvConn& candidate = *i;
                            if( candidate.second == resultConn )
                            {
                                Receiver* receiver = candidate.first;
                                _receivers.erase( i );
                                if( _useThreads )
                                {
                                    receiver->stop();
                                    receiver->join();
                                }
                                delete receiver;
                                break;
                            }
                        }

                        cerr << --_nClients << " clients" << endl;
                        break;

                    case ConnectionSet::EVENT_INTERRUPT:
                        break;

                    default:
                        TESTINFO( false, "Not reachable" );
                }
            }
            TESTINFO( _receivers.empty(), _receivers.size() );
            TESTINFO( _connectionSet.getSize() == 1, _connectionSet.getSize( ));
            _connectionSet.clear();
            return EXIT_SUCCESS;
        }
private:
    typedef std::pair< Receiver*, ConnectionPtr > RecvConn;
    ConnectionPtr    _connection;
    vector< RecvConn > _receivers;
    size_t _packetSize;
    bool _useThreads;
    eq::base::Bufferb _buffer;
};

}


int main( int argc, char **argv )
{
    TEST( eq::net::init( argc, argv ));

    ConnectionDescriptionPtr description = new ConnectionDescription;
    description->type = CONNECTIONTYPE_TCPIP;
    description->port = 4242;

    bool isClient     = true;
    bool useThreads   = false;
    size_t packetSize = 1048576;
    size_t nPackets   = 0xffffffffu;
    size_t waitTime   = 0;

    try // command line parsing
    {
        TCLAP::CmdLine command( "netPerf - Equalizer network benchmark tool\n");
        TCLAP::ValueArg<string> clientArg( "c", "client", "run as client", 
                                           true, "", "IP[:port]" );
        TCLAP::ValueArg<string> serverArg( "s", "server", "run as server", 
                                           true, "", "IP[:port]" );
        TCLAP::SwitchArg threadedArg( "t", "threaded", 
                          "Run each receive in a separate thread (server only)",
                                      command, false );
        TCLAP::ValueArg<size_t> sizeArg( "p", "packetSize", "packet size", 
                                         false, packetSize, "unsigned", 
                                         command );
        TCLAP::ValueArg<size_t> packetsArg( "n", "numPackets", 
                                            "number of packets to send", 
                                            false, nPackets, "unsigned",
                                            command );
        TCLAP::ValueArg<size_t> waitArg( "w", "wait", 
                                   "wait time (ms) between sends (client only)",
                                         false, 0, "unsigned", command );

        command.xorAdd( clientArg, serverArg );
        command.parse( argc, argv );

        if( clientArg.isSet( ))
            description->fromString( clientArg.getValue( ));
        else if( serverArg.isSet( ))
        {
            isClient = false;
            description->fromString( serverArg.getValue( ));
        }

        useThreads = threadedArg.isSet();

        if( sizeArg.isSet( ))
            packetSize = sizeArg.getValue();
        if( packetsArg.isSet( ))
            nPackets = packetsArg.getValue();
        if( waitArg.isSet( ))
            waitTime = waitArg.getValue();
    }
    catch( TCLAP::ArgException& exception )
    {
        EQERROR << "Command line parse error: " << exception.error() 
                << " for argument " << exception.argId() << endl;

        eq::net::exit();
        return EXIT_FAILURE;
    }

    // run
    ConnectionPtr connection = Connection::create( description );
    Selector* selector = 0;
    if( isClient )
    {
        if( description->type == CONNECTIONTYPE_RSP)
        {
            selector = new Selector( connection, packetSize, useThreads );
            selector->start();
        }
        else
            TEST( connection->connect( ));

        eq::base::Buffer< uint8_t > buffer;
        buffer.resize( packetSize );
        for( size_t i = 0; i<packetSize; ++i )
            buffer[i] = static_cast< uint8_t >( i );

        const float mBytesSec = buffer.getSize() / 1024.0f / 1024.0f * 1000.0f;
        eq::base::Clock clock;
        size_t      lastOutput = nPackets;

        clock.reset();
        while( nPackets-- )
        {
            buffer.getData()[0] = nPackets;
            TEST( connection->send( buffer.getData(), buffer.getSize() ));
            const float time = clock.getTimef();
            if( time > 1000.f )
            {
                const eq::base::ScopedMutex mutex( _mutexPrint );
                const size_t nSamples = lastOutput - nPackets;
                cerr << static_cast<uint32_t>( nPackets ) 
                     << " Send perf: " << mBytesSec / time * nSamples 
                     << "MB/s (" << nSamples / time * 1000.f  << "pps)" << endl;

                lastOutput = nPackets;
                clock.reset();
            }
            if( waitTime > 0 )
                eq::base::sleep( waitTime );
        }
        const float time = clock.getTimef();
        const size_t nSamples = lastOutput - nPackets;
        if( nSamples != 0 )
        {
            const eq::base::ScopedMutex mutex( _mutexPrint );
            cerr << "Send perf: " << mBytesSec / time * nSamples 
                 << "MB/s (" << nSamples / time * 1000.f  << "pps)" << endl;
        }
    }
    else
    {
        selector = new Selector( connection, packetSize, useThreads );
        selector->start();
    }

    if ( selector )
        selector->join();
    delete selector;

    TESTINFO( connection->getRefCount() == 1, connection->getRefCount( ));
    connection->close();
    return EXIT_SUCCESS;
}

