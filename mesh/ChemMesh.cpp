/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"
#include "ElementValueFinfo.h"
#include "Boundary.h"
#include "MeshEntry.h"
#include "Stencil.h"
#include "ChemMesh.h"
#include "../ksolve/StoichHeaders.h"

// Not used anywhere currently. - Sid
static SrcFinfo0 groupSurfaces( 
		"groupSurfaces", 
		"Goes to all surfaces that define this ChemMesh"
);

SrcFinfo5< 
	double,
	vector< double >,
	vector< unsigned int>, 
	vector< vector< unsigned int > >, 
	vector< vector< unsigned int > >
	>* meshSplit()
{
	static SrcFinfo5< 
			double,
			vector< double >,
			vector< unsigned int >, 
			vector< vector< unsigned int > >, 
			vector< vector< unsigned int > >
		>
	meshSplit(
		"meshSplit",
		"Defines how meshEntries communicate between nodes."
		"Args: oldVol, volListOfAllEntries, localEntryList, "
		"outgoingDiffusion[node#][entry#], incomingDiffusion[node#][entry#]"
		"This message is meant to go to the SimManager and Stoich."
	);
	return &meshSplit;
}

static SrcFinfo2< unsigned int, vector< double > >* meshStats()
{
	static SrcFinfo2< unsigned int, vector< double > > meshStats(
		"meshStats",
		"Basic statistics for mesh: Total # of entries, and a vector of"
		"unique volumes of voxels"
	);
	return &meshStats;
}

const Cinfo* ChemMesh::initCinfo()
{
		//////////////////////////////////////////////////////////////
		// Field Definitions
		//////////////////////////////////////////////////////////////
		static ElementValueFinfo< ChemMesh, double > size(
			"size",
			"Size of entire chemical domain."
			"Assigning this assumes that the geometry is that of the "
			"default mesh, which may not be what you want. If so, use"
			"a more specific mesh assignment function.",
			&ChemMesh::setEntireSize,
			&ChemMesh::getEntireSize
		);

		static ReadOnlyValueFinfo< ChemMesh, unsigned int > numDimensions(
			"numDimensions",
			"Number of spatial dimensions of this compartment. Usually 3 or 2",
			&ChemMesh::getDimensions
		);

		static ValueFinfo< ChemMesh, string > method(
			"method",
			"Advisory field for SimManager to check when assigning "
			"solution methods. Doesn't do anything unless SimManager scans",
			&ChemMesh::setMethod,
			&ChemMesh::getMethod
		);

		//////////////////////////////////////////////////////////////
		// MsgDest Definitions
		//////////////////////////////////////////////////////////////

		static DestFinfo group( "group",
			"Handle for grouping. Doesn't do anything.",
			new OpFuncDummy() );

		/*
		static DestFinfo stoich( "stoich",
			"Handle Id of stoich. Used to set up connection from mesh to"
			"stoich for diffusion "
			"calculations. Somewhat messy way of doing it, should really "
			"use messaging.",
			new EpFunc1< ChemMesh, Id >( &ChemMesh::stoich )
		);
		*/

		static DestFinfo buildDefaultMesh( "buildDefaultMesh",
			"Tells ChemMesh derived class to build a default mesh with the"
			"specified size and number of meshEntries.",
			new EpFunc2< ChemMesh, double, unsigned int >( 
				&ChemMesh::buildDefaultMesh )
		);

		static DestFinfo handleRequestMeshStats( "handleRequestMeshStats",
			"Handles request from SimManager for mesh stats",
			new EpFunc0< ChemMesh >(
				&ChemMesh::handleRequestMeshStats
			)
		);

		static DestFinfo handleNodeInfo( "handleNodeInfo",
			"Tells ChemMesh how many nodes and threads per node it is "
			"allowed to use. Triggers a return meshSplit message.",
			new EpFunc2< ChemMesh, unsigned int, unsigned int >(
				&ChemMesh::handleNodeInfo )
		);

		static DestFinfo resetStencil( "resetStencil",
			"Resets the diffusion stencil to the core stencil that only "
			"includes the within-mesh diffusion. This is needed prior to "
			"building up the cross-mesh diffusion through junctions.",
			new OpFunc0< ChemMesh >(
				&ChemMesh::resetStencil )
		);


		//////////////////////////////////////////////////////////////
		// SharedMsg Definitions
		//////////////////////////////////////////////////////////////

		static Finfo* nodeMeshingShared[] = {
			meshSplit(), meshStats(), 
			&handleRequestMeshStats, &handleNodeInfo
		};

		static SharedFinfo nodeMeshing( "nodeMeshing",
			"Connects to SimManager to coordinate meshing with parallel"
			"decomposition and with the Stoich",
			nodeMeshingShared, sizeof( nodeMeshingShared ) / sizeof( const Finfo* )
		);

		/*
		static Finfo* geomShared[] = {
			&requestSize, &handleSize
		};

		static SharedFinfo geom( "geom",
			"Connects to Geometry tree(s) defining compt",
			geomShared, sizeof( geomShared ) / sizeof( const Finfo* )
		);
		*/

		//////////////////////////////////////////////////////////////
		// Field Elements
		//////////////////////////////////////////////////////////////
		static FieldElementFinfo< ChemMesh, Boundary > boundaryFinfo( 
			"boundary", 
			"Field Element for Boundaries",
			Boundary::initCinfo(),
			&ChemMesh::lookupBoundary,
			&ChemMesh::setNumBoundary,
			&ChemMesh::getNumBoundary,
			4
		);

		static FieldElementFinfo< ChemMesh, MeshEntry > entryFinfo( 
			"mesh", 
			"Field Element for mesh entries",
			MeshEntry::initCinfo(),
			&ChemMesh::lookupEntry,
			&ChemMesh::setNumEntries,
			&ChemMesh::getNumEntries,
			1
		);

	static Finfo* chemMeshFinfos[] = {
		&size,			// Value
		&numDimensions,	// ReadOnlyValue
		&method,		// Value
		&buildDefaultMesh,	// DestFinfo
		&resetStencil,	// DestFinfo
		&nodeMeshing,	// SharedFinfo
		&entryFinfo,	// FieldElementFinfo
		&boundaryFinfo,	// FieldElementFinfo
	};

	static Cinfo chemMeshCinfo (
		"ChemMesh",
		Neutral::initCinfo(),
		chemMeshFinfos,
		sizeof( chemMeshFinfos ) / sizeof ( Finfo* ),
		new Dinfo< short >()
	);

	return &chemMeshCinfo;
}

//////////////////////////////////////////////////////////////
// Basic class Definitions
//////////////////////////////////////////////////////////////

static const Cinfo* chemMeshCinfo = ChemMesh::initCinfo();

ChemMesh::ChemMesh()
	: 
		size_( 1.0 ),
		entry_( this )
{
	;
}

ChemMesh::~ChemMesh()
{ 
	for ( unsigned int i = 0; i < stencil_.size(); ++i ) {
		if ( stencil_[i] )
			delete stencil_[i];
	}
}

//////////////////////////////////////////////////////////////
// MsgDest Definitions
//////////////////////////////////////////////////////////////

void ChemMesh::buildDefaultMesh( const Eref& e, const Qinfo* q,
	double size, unsigned int numEntries )
{
	this->innerBuildDefaultMesh( e, q, size, numEntries );
}

void ChemMesh::handleRequestMeshStats( const Eref& e, const Qinfo* q )
{
	// Pass it down to derived classes along with the SrcFinfo
	innerHandleRequestMeshStats( e, q, meshStats() );
}

void ChemMesh::handleNodeInfo( const Eref& e, const Qinfo* q,
	unsigned int numNodes, unsigned int numThreads )
{
	// Pass it down to derived classes along with the SrcFinfo
	innerHandleNodeInfo( e, q, numNodes, numThreads );
}

void ChemMesh::resetStencil()
{
	this->innerResetStencil();
}

//////////////////////////////////////////////////////////////
// Field Definitions
//////////////////////////////////////////////////////////////

double ChemMesh::getEntireSize( const Eref& e, const Qinfo* q ) const
{
	return size_;
}

void ChemMesh::setEntireSize( const Eref& e, const Qinfo* q, double size )
{
	buildDefaultMesh( e, q, size, getNumEntries() );
}

unsigned int ChemMesh::getDimensions() const
{
	return this->innerGetDimensions();
}

string ChemMesh::getMethod() const
{
	return method_;
}

void ChemMesh::setMethod( string method )
{
	method_ = method;
}

//////////////////////////////////////////////////////////////
// Element Field Definitions
//////////////////////////////////////////////////////////////

MeshEntry* ChemMesh::lookupEntry( unsigned int index )
{
	return &entry_;
}

void ChemMesh::setNumEntries( unsigned int num )
{
	this->innerSetNumEntries( num );
	// cout << "Warning: ChemMesh::setNumEntries: No effect. Use subclass-specific functions\nto build or resize mesh.\n";
}

unsigned int ChemMesh::getNumEntries() const
{
	return this->innerGetNumEntries();
}

//////////////////////////////////////////////////////////////
// Element Field Definitions for boundary
//////////////////////////////////////////////////////////////

Boundary* ChemMesh::lookupBoundary( unsigned int index )
{
	if ( index < boundaries_.size() )
		return &( boundaries_[index] );
	cout << "Error: ChemCompt::lookupBoundary: Index " << index << 
		" >= vector size " << boundaries_.size() << endl;
	return 0;
}

void ChemMesh::setNumBoundary( unsigned int num )
{
	assert( num < 1000 ); // Pretty unlikely upper limit
	boundaries_.resize( num );
}

unsigned int ChemMesh::getNumBoundary() const
{
	return boundaries_.size();
}

//////////////////////////////////////////////////////////////
// Build the junction between this and another ChemMesh.
// This one function does the work for both meshes.
//////////////////////////////////////////////////////////////
void ChemMesh::buildJunction( ChemMesh* other, vector< VoxelJunction >& ret)
{
	matchMeshEntries( other, ret );
	extendStencil( other, ret );
	/*
	 * No longer having diffusion to abutting voxels in the follower
	 * compartment.
	 *
	flipRet( ret );
	other->extendStencil( this, ret );
	flipRet( ret );
	*/
}

void ChemMesh::flipRet( vector< VoxelJunction >& ret ) const
{
   vector< VoxelJunction >::iterator i;
   for ( i = ret.begin(); i != ret.end(); ++i ) {
		  unsigned int temp = i->first;
		  i->first = i->second;
		  i->second = temp;
   }
}

//////////////////////////////////////////////////////////////
// Orchestrate diffusion calculations in Stoich. This simply updates
// the flux terms (du/dt due to diffusion). Virtual func, has to be
// defined for every Mesh class if it differs from below.
// Called from the MeshEntry.
//////////////////////////////////////////////////////////////

void ChemMesh::lookupStoich( ObjId me ) const
{
	ChemMesh* cm = reinterpret_cast< ChemMesh* >( me.data() );
	assert( cm == this );
	vector< Id > stoichVec;
	unsigned int num = me.element()->getNeighbours( stoichVec, meshSplit());
	if ( num == 1 ) // The solver has been created
		cm->stoich_ = stoichVec[0];
}

void ChemMesh::updateDiffusion( unsigned int meshIndex ) const
{
	// Later we'll have provision for multiple stoich targets.
	if ( stoich_ != Id() ) {
		Stoich* s = reinterpret_cast< Stoich* >( stoich_.eref().data() );
		s->updateDiffusion( meshIndex, stencil_ );
	}
}
