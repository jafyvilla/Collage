
/* Copyright (c) 2007-2009, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#ifndef EQNET_OBJECTCM_H
#define EQNET_OBJECTCM_H

#include <eq/net/dispatcher.h>   // base class
#include <eq/net/types.h>

namespace eq
{
namespace net
{
    class Node;
    class Object;

    /** 
     * The object change manager base class.
     *
     * Each object has a change manager to create and store version
     * information. The type of change manager depends on the object
     * implementation and if it is the master object or a slave object.
     */
    class ObjectCM : public Dispatcher
    {
    public:
        /** Construct a new change manager. */
        ObjectCM() {}
        virtual ~ObjectCM() {}

        /** 
         * Make this object thread safe.
         * 
         * The caller has to ensure that no other thread is using this object
         * when this function is called. It is primarily used by the session
         * during object instanciation.
         * @sa Session::getObject().
         */
        virtual void makeThreadSafe() = 0;

        /**
         * @name Versioning
         */
        //*{
        /** 
         * Start committing a new version.
         * 
         * @return the commit identifier to be passed to commitSync
         * @sa commitSync
         */
        virtual uint32_t commitNB() = 0;
        
        /** 
         * Finalize a commit transaction.
         * 
         * @param commitID the commit identifier returned from commitNB
         * @return the new head version.
         */
        virtual uint32_t commitSync( const uint32_t commitID ) = 0;

        /** 
         * Explicitily obsolete all versions.
         * 
         * @param version the version to obsolete
         */
        virtual void obsolete( const uint32_t version ) = 0;

        /** 
         * Automatically obsolete old versions.
         * 
         * @param count the number of versions to retain, excluding the head
         *              version.
         * @param flags additional flags for the auto-obsoletion mechanism
         */
        virtual void setAutoObsolete( const uint32_t count,
                                      const uint32_t flags ) = 0;
 
        /** @return get the number of versions this object retains. */
        virtual uint32_t getAutoObsoleteCount() const = 0;

        /** 
         * Sync to a given version.
         *
         * @param version the version to synchronize, must be bigger than the
         *                current version.
         * @return the version of the object after the operation.
         */
        virtual uint32_t sync( const uint32_t version ) = 0;

        /** @return the latest available (head) version. */
        virtual uint32_t getHeadVersion() const = 0;

        /** @return the current version. */
        virtual uint32_t getVersion() const = 0;

        /** @return the oldest available version. */
        virtual uint32_t getOldestVersion() const = 0;
        //*}

        /** @return if this instance is the master version. */
        virtual bool isMaster() const = 0;

        /** @return the instance identifier of the master object. */
        virtual uint32_t getMasterInstanceID() const = 0;

        /** 
         * Add a subscribed slave to the managed object.
         * 
         * @param node the slave node.
         * @param instanceID the object instance identifier on the slave node.
         * @param version the initial version.
         */
        virtual void addSlave( NodePtr node, const uint32_t instanceID,
                               const uint32_t version ) = 0;

        /** 
         * Remove a subscribed slave.
         * 
         * @param node the slave node. 
         */
        virtual void removeSlave( NodePtr node ) = 0;

        /** Apply the initial data after mapping. */
        virtual void applyMapData() = 0;

        /** Add the old master as a slave. */
        virtual void addOldMaster( NodePtr node, const uint32_t instanceID ) =0;

        /** The default CM for unattached objects. */
        static ObjectCM* ZERO;
    };
}
}

#endif // EQNET_OBJECTCM_H
