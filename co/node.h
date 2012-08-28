
/* Copyright (c) 2005-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
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

#ifndef CO_NODE_H
#define CO_NODE_H

#include <co/dispatcher.h>        // base class
#include <co/commands.h>          // for COMMANDTYPE_CO_NODE enum
#include <co/connection.h>        // used in inline template method
#include <co/nodeType.h>          // for NODETYPE_CO_NODE enum
#include <co/types.h>

namespace co
{
namespace detail { class Node; }

    /**
     * Manages a node.
     *
     * A node represents a separate entity in a peer-to-peer network, typically
     * a process on a cluster node or on a shared-memory system. It should have
     * at least one Connection through which is reachable. A Node provides the
     * basic communication facilities through message passing.
     */
    class Node : public Dispatcher, public lunchbox::Referenced
    {
    public:
        /** Construct a new Node. */
        CO_API Node();

        /** @name Data Access. */
        //@{
        bool operator == ( const Node* n ) const;

        CO_API bool isReachable() const;
        CO_API bool isConnected() const;
        CO_API bool isClosed() const;
        CO_API bool isClosing() const;
        CO_API bool isListening() const;
        //@}

        /** @name Connectivity information. */
        //@{
        /** @return true if the node is local (listening), false otherwise. */
        bool isLocal() const { return isListening(); }

        /** 
         * Adds a new description how this node can be reached.
         * 
         * @param cd the connection description.
         */
        CO_API void addConnectionDescription( ConnectionDescriptionPtr cd );
        
        /** 
         * Removes a connection description.
         * 
         * @param cd the connection description.
         * @return true if the connection description was removed, false
         *         otherwise.
         */
        CO_API bool removeConnectionDescription( ConnectionDescriptionPtr cd );

        /** @return the number of stored connection descriptions. */
        CO_API ConnectionDescriptions getConnectionDescriptions() const;

        /** @return the connection to this node. */
        CO_API ConnectionPtr getConnection() const;

        /** @return the established multicast connection to this node. */
        ConnectionPtr getMulticast() const;

        /** @return the first usable multicast connection to this node, or 0. */
        ConnectionPtr useMulticast();
        //@}

        /** @name Messaging API */
        //@{
        /**
         * Send a command with optional data to the node.
         *
         * The returned data stream can be used to pass additional data to the
         * given command. The data will be send after the stream is destroyed,
         * aka when it is running out of scope.
         *
         * @param cmd the node command to execute
         * @param type the type of object that should handle this command
         * @param multicast prefer multicast connection for sending
         * @return the stream object to pass additional data to
         */
        CO_API NodeOCommand send( uint32_t cmd,
                                  uint32_t type = COMMANDTYPE_CO_NODE,
                                  bool multicast = false );
        //@}

        CO_API const NodeID& getNodeID() const;

        /** Serialize the node's information. */
        CO_API std::string serialize() const;
        /** Deserialize the node information, consumes given data. */
        CO_API bool deserialize( std::string& data );

        /** @return last receive time. */
        int64_t getLastReceiveTime() const;

        /** @return the type of the node, used during connect(). */
        virtual uint32_t getType() const { return NODETYPE_CO_NODE; }

    protected:
        /** Destructs this node. */
        CO_API virtual ~Node();

        /** 
         * Factory method to create a new node.
         * 
         * @param type the type the node type
         * @return the node.
         * @sa getType()
         */
        CO_API virtual NodePtr createNode( const uint32_t type );

    private:
        detail::Node* const _impl;
        CO_API friend std::ostream& operator << ( std::ostream&, const Node& );

        /** Ensures the connectivity of this node. */
        CO_API ConnectionPtr _getConnection();

        /** @internal @name Methods for LocalNode */
        //@{
        void _addMulticast( NodePtr node, ConnectionPtr connection );
        void _removeMulticast( ConnectionPtr connection );
        void _connectMulticast( NodePtr node );
        void _connectMulticast( NodePtr node, ConnectionPtr connection );
        void _setListening();
        void _setClosing();
        void _setClosed();
        void _connect( ConnectionPtr connection );
        void _disconnect();
        void _setLastReceive( const int64_t time );
        friend class LocalNode;
        //@}
    };

    CO_API std::ostream& operator << ( std::ostream& os, const Node& node );
}

namespace lunchbox
{
template<> inline void byteswap( co::Node*& value ) { /*NOP*/ }
}

#endif // CO_NODE_H