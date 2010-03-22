
/* Copyright (c) 2008-2010, Stefan Eilemann <eile@equalizergraphics.com> 
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

#ifndef EQFABRIC_VIEW_H
#define EQFABRIC_VIEW_H

#include <eq/fabric/frustum.h>        // base class
#include <eq/fabric/types.h>
#include <eq/fabric/viewport.h>       // member
#include <eq/fabric/visitorResult.h>  // enum

namespace eq
{
namespace fabric
{
    /**
     * A View is a 2D area of a Layout. It is a view of the application's data
     * on a model, in the sense used by the MVC pattern. It can be a scene,
     * viewing mode, viewing position, or any other representation of the
     * application's data.
     */
    template< class L, class V, class O > class View : public Frustum
    {
    public:
        /** @name Data Access. */
        //@{
        /** @return the viewport of the view wrt its layout. @version 1.0 */
        EQFABRIC_EXPORT const Viewport& getViewport() const;

        /** @return the parent layout of this view. @version 1.0 */
        EQFABRIC_EXPORT L* getLayout() { return _layout; }

        /** @return the parent layout of this view. @version 1.0 */
        EQFABRIC_EXPORT const L* getLayout() const { return _layout; }

        /**
         * @return the observer tracking this view, or 0 for untracked views.
         * @version 1.0
         */
        O* getObserver() { return _observer; }

        /** const version of getObserver(). @version 1.0 */
        const O* getObserver() const { return _observer; }

        /** @warning  Undocumented - may not be supported in the future */
        EQFABRIC_EXPORT void setOverdraw( const Vector2i& pixels );

        /** @warning  Undocumented - may not be supported in the future */
        const Vector2i& getOverdraw() const { return _overdraw; }

        /** Set the 2D viewport wrt Layout and Canvas. @internal */
        void setViewport( const Viewport& viewport );

        /** Set the entity tracking this view. @internal */
        void setObserver( O* observer );
        //@}

        /** @name Operations */
        //@{
        /** 
         * Traverse this view using a view visitor.
         * 
         * @param visitor the visitor.
         * @return the result of the visitor traversal.
         * @version 1.0
         */
        EQFABRIC_EXPORT VisitorResult accept( LeafVisitor< V >& visitor );

        /** Const-version of accept(). @version 1.0 */
        EQFABRIC_EXPORT VisitorResult accept( LeafVisitor< V >& visitor ) const;
        //@}

    protected:
        enum DirtyBits
        {
            DIRTY_VIEWPORT   = Frustum::DIRTY_CUSTOM << 0,
            DIRTY_OBSERVER   = Frustum::DIRTY_CUSTOM << 1,
            DIRTY_OVERDRAW   = Frustum::DIRTY_CUSTOM << 2
        };

        /** Construct a new view. @internal */
        EQFABRIC_EXPORT View( L* layout );

        /** Construct a new deep copy of a view. @internal */
        EQFABRIC_EXPORT View( const View& from, L* layout );

        /** Destruct this view. @internal */
        EQFABRIC_EXPORT virtual ~View();

        /** @sa Frustum::serialize() */
        EQFABRIC_EXPORT virtual void serialize( net::DataOStream& os,
                                                const uint64_t dirtyBits );

        /** @sa Frustum::deserialize() */
        EQFABRIC_EXPORT virtual void deserialize( net::DataIStream& is, 
                                                  const uint64_t dirtyBits );
    private:
        /** Parent layout (application-side). */
        L* const _layout;

        /** The observer for tracking. */
        O* _observer;

        /** Logical 2D area of Canvas covered. */
        Viewport _viewport;

        /** Enlarge size of all dest channels and adjust frustum accordingly. */
        Vector2i _overdraw;

        union // placeholder for binary-compatible changes
        {
            char dummy[32];
        };
    };

    template< class L, class V, class O >
    std::ostream& operator << ( std::ostream& os, const View< L, V, O >& view );
}
}
#endif // EQFABRIC_VIEW_H
