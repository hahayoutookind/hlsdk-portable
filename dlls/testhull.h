#include "nodes.h"

class CTestHull : public CBaseMonster
{
public:
        void Spawn( entvars_t *pevMasterNode );
        virtual int ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
        void EXPORT CallBuildNodeGraph ( void );
        void BuildNodeGraph( void );
        void EXPORT ShowBadNode( void );
        void EXPORT DropDelay( void );
        void EXPORT PathFind( void );

        Vector vecBadNodeOrigin;
};
