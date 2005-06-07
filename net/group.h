
/* Copyright (c) 2005, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

namespace eqNet
{
    /**
     * A Group is a collection of nodes for broadcast communications.
     *
     * Depending on the used network protocol, these communication methods may
     * used an optimised network transport mechanism.
     *
     * @sa Node
     */
    class Group
    {
    public:
        //void               broadCast( uint gid_to, Type type, void *ptr,
        //uint64 count, uint flags );

        /** 
         * Performs a barrier operation with all members of this group.
         * 
         * @param groupID the identifier of the group.
         */
        static void barrier( const uint groupID );
    };
};


