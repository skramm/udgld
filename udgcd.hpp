// Copyright Sebastien Kramm, 2016-2017
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
\file udgcd.hpp
\brief UnDirected Graph Cycle Detection. Finds all the cycles inside an undirected graph.

Home page: https://github.com/skramm/udgcd

Inspired from http://www.boost.org/doc/libs/1_58_0/libs/graph/example/undirected_dfs.cpp


See file README.md
*/

#ifndef HG_UDGCD_HPP
#define HG_UDGCD_HPP

#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/undirected_dfs.hpp>
#include <boost/graph/connected_components.hpp>

//#define DEV_MODE

/// All the provided code is in this namespace
namespace udgcd {

//-------------------------------------------------------------------------------------------
/// Private, don't use.
/**
Recursive function, explores edges connected to \c v1 until we find a cycle

\warning Have to be sure there \b is a cycle, else infinite recursion !
*/
template <class Vertex, class Graph>
bool
explore(
	const Vertex& v1,                            ///< the starting vertex we want to explore
	const Graph&  g,
	std::vector<std::vector<Vertex>>& vv_paths,
	std::vector<std::vector<Vertex>>& v_cycles, ///< this is where we store the paths that have cycles
	int depth = 0
) {
	++depth;
	static int max_depth = std::max( depth, max_depth );
	assert( vv_paths.size()>0 );

	typename boost::graph_traits<Graph>::out_edge_iterator ei, ei_end;
	boost::tie(ei, ei_end) = out_edges( v1, g );
//	COUTP << "nb of edges = " << ei_end - ei << "\n";
	size_t edge_idx = 0;

	std::vector<Vertex> src_path = vv_paths[vv_paths.size()-1];
//	COUTP << "src_path :"; for( const auto& vv:src_path )	cout << vv << "-"; cout << "\n";

	bool found = false;
//	int iter=0;
//	size_t n = ei_end -ei;
	for( ; ei != ei_end; ++ei, ++edge_idx )
	{
		bool b = false;
		Vertex v2a = source(*ei, g);
		Vertex v2b = target(*ei, g);
//		COUTP << ++iter << '/' <<  n << ": connected edges v2: v2a=" << v2a << " v2b=" << v2b << "\n";

		if( v2b == v1 && v2a == src_path[0] ) // we just found the edge that we started on, so no need to finish the current iteration, just move on.
			continue;

		std::vector<Vertex> newv(src_path);
		bool AddNode = true;
		if( newv.size() > 1 )
			if( newv[ newv.size()-2 ] == v2b )
				AddNode = false;

		if( AddNode )
		{
			if( std::find( newv.cbegin(), newv.cend(), v2b ) != newv.cend() )
			{
				newv.push_back( v2b );
//				COUTP << "*** FOUND CYCLE!\n";
				v_cycles.push_back( newv );
				return true;
			}
			else
			{
				newv.push_back( v2b );         // else add'em and continue
//				COUTP << "  -adding vector ";  for( const auto& vv:newv )	cout << vv << "-"; cout << "\n";
				vv_paths.push_back( newv );
				b = explore( v2b, g, vv_paths, v_cycles, depth );
			}
		}
		if( b )
			found = true;
	}
	return found;
}
//-------------------------------------------------------------------------------------------
template<typename T>
void
PrintVector( std::ostream& f, const std::vector<T>& vec )
{
	for( const auto& elem : vec )
		f << elem << "-";
	f << "\n";
}

/// Additional helper function, can be used to print the cycles found
template<typename T>
void
PrintPaths( std::ostream& f, const std::vector<std::vector<T>>& v_paths, const char* msg=0 )
{
	static int iter=0;
	f << "Paths (" << iter++ << "): nb=" << v_paths.size();
	if( msg )
		f << ": " << msg;
	f << "\n";

	for( size_t i=0; i<v_paths.size(); i++ )
	{
		f << " - " << i << ": ";
		PrintVector( f, v_paths[i] );
	}
}
//-------------------------------------------------------------------------------------------
/// Private, don't use.
/**
 Remove twins : vector that are the same, but in reverse order
*/
template<typename T>
std::vector<std::vector<T>>
RemoveOppositePairs( const std::vector<std::vector<T>>& v_cycles )
{
	assert( v_cycles.size() );
	std::vector<std::vector<T>> out;      // output vector
	std::vector<bool> flags( v_cycles.size(), true ); // some flags to keep track of which elements are reversed

	for( size_t i=0; i<v_cycles.size()-1; ++i )
		if( flags[i] )
		{
			std::vector<T> rev = v_cycles[i];                       // step 1: build a reversed copy of the current vector
			std::reverse( rev.begin(), rev.end() );
			for( size_t j=i+1; j<v_cycles.size(); ++j )                  // step 2: parse the rest of the list, and check
				if( flags[j] && rev == v_cycles[j] )                     // if similar, then
				{
					out.push_back( v_cycles[i] );                        //  1 - add current vector into output
					flags[j] = false;                                 //  2 -  invalidate the reversed one
				}
		}
	return out;
}

//-------------------------------------------------------------------------------------------
template<typename T>
void
PutSmallestElemFirst( std::vector<T>& vec )
{
	auto it = std::min_element( vec.begin(), vec.end() );     // rotate so that smallest is first
	std::rotate( vec.begin(), it, vec.end() );
}
//-------------------------------------------------------------------------------------------
/// Private, don't use.
/**
Helper function for RemoveIdentical()

Given an input vector "DABCD", it will return "ABCD" (removal of duplicate element, and first element is the smallest)
*/
template<typename T>
std::vector<T>
GetSortedTrimmed( const std::vector<T>& v_in )
{
	assert( v_in.front() == v_in.back() ); // check this is a cycle
	assert( v_in.size() > 2 );            // a (complete) cycle needs to be at least 3 vertices long

	std::vector<T> v_out( v_in.size() - 1 );                      // Trim: remove
	std::copy( v_in.cbegin(), v_in.cend()-1, v_out.begin() );     // last element

	PutSmallestElemFirst( v_out );

	if( v_out.back() < v_out[1] )                     // if we have 1-4-3-2, then
	{
		std::reverse( v_out.begin(), v_out.end() );   // we transform it into 2-3-4-1
		PutSmallestElemFirst( v_out );                // and put smallest first: 1-2-3-4
	}
	return v_out;
}

//-------------------------------------------------------------------------------------------
/// Private, don't use.
/**
Remove identical strings that are the same up to the starting point
It also sorts the paths by rotating them so that the node of smallest index is first
*/
template<typename T>
std::vector<std::vector<T>>
RemoveIdentical( const std::vector<std::vector<T>>& v_cycles )
{
	assert( v_cycles.size() );

	if( v_cycles.size() == 1 )                                   // if single path in input, then we justs add it, after trimming/sorting
	{
		std::vector<std::vector<T>> out( 1, GetSortedTrimmed( v_cycles[0] ) );
		return out;
	}

	std::vector<std::vector<T>> out( v_cycles.size() );
	for( size_t i=0; i<v_cycles.size(); i++ )            // 1 - fill output vector with sorted/trimmed paths
		out[i] = GetSortedTrimmed( v_cycles[i] );

	std::sort( out.begin(), out.end() );                 // 2 - sort
	out.erase(                                           // 3 - erase the ones that are
		std::unique( out.begin(), out.end() ),           //  consecutive duplicates
		out.end()
	);

	return out;
}

//-------------------------------------------------------------------------------------------
/// Returns true if vertices \c v1 and \c v2 are connected by an edge
/**
http://www.boost.org/doc/libs/1_59_0/libs/graph/doc/IncidenceGraph.html#sec:out-edges
*/
template<typename vertex_t, typename graph_t>
bool
AreConnected( const vertex_t& v1, const vertex_t& v2, const graph_t& g )
{
	auto pair_edge = boost::out_edges( v1, g );                      // get iterator range on edges
	for( auto it = pair_edge.first; it != pair_edge.second; ++it )
		if( v2 == boost::target( *it, g ) )
			return true;
	return false;
}
//-------------------------------------------------------------------------------------------
/// Return true if cycle is chordless
/**
See
- https://en.wikipedia.org/wiki/Cycle_(graph_theory)#Chordless_cycles

Quote:
"A chordless cycle in a graph, also called a hole or an induced cycle, is a cycle such that
no two vertices of the cycle are connected by an edge that does not itself belong to the cycle."
*/
template<typename vertex_t, typename graph_t>
bool
IsChordless( const std::vector<vertex_t>& path, const graph_t& g )
{
	if( path.size() < 4 ) // else, no need to test
		return true;

	for( size_t i=0; i<path.size()-3; ++i )
	{
		for( size_t j=i+2; j<path.size()-1; ++j )

		if( AreConnected( path[i], path[j], g ) )
			return false;
	}
	return true;
}
//-------------------------------------------------------------------------------------------
/// Third step, remove non-chordless cycles
template<typename vertex_t, typename graph_t>
std::vector<std::vector<vertex_t>>
RemoveNonChordless( const std::vector<std::vector<vertex_t>>& v_in, const graph_t& g )
{
	std::vector<std::vector<vertex_t>> v_out;
	v_out.reserve( v_in.size() ); // to avoid unnecessary memory reallocations and copies
    for( const auto& cycle: v_in )
		if( IsChordless( cycle, g ) )
			v_out.push_back( cycle );
	return v_out;
}

//-------------------------------------------------------------------------------------------
/// holds two vertices, used in RemoveRedundant()
template <typename vertex_t>
struct VertexPair
{
	vertex_t v1,v2;
	VertexPair( vertex_t va, vertex_t vb ): v1(va), v2(vb)
	{
		if( v2<v1 )
			std::swap( v1, v2 );
	}
	// 2-3 is smaller than 2-4
	friend bool operator < ( const VertexPair& vp_a, const VertexPair& vp_b )
	{
//		assert( vp_a.v1 < vp_a.v2 );
		if( vp_a.v1 < vp_b.v1 ) //&& vp_a.v2 < vp_b.v2 )
				return true;
		else
		{
			if( vp_a.v1 == vp_b.v1 )
				return ( vp_a.v2 < vp_b.v2 );
		}
//			if( vp_a.v2 < vp_b.v2 )
//				return true;
		return false;
	}

#ifndef DEV_MODE
	friend std::ostream& operator << ( std::ostream& s, const VertexPair& vp )
	{
		s << '(' << vp.v1 << '-' << vp.v2 << ')';
		return s;
	}
#endif
};
//-------------------------------------------------------------------------------------------
template<typename vertex_t>
void
PrintSet( const std::set<VertexPair<vertex_t>>& set_edges, std::string msg )
{
	std::cout << "set: " << msg << '\n';
	for( const auto& e: set_edges )
		std::cout << e << '-';
	std::cout << '\n';
}
//-------------------------------------------------------------------------------------------
/// Post-process step: removes paths (cycles) that are redundant (i.e. that can be deduced/build from the others)
/**
arg is not const, because it gets sorted here.
\bug bug here, see RemoveRedundant2
*/
template<typename vertex_t, typename graph_t>
std::vector<std::vector<vertex_t>>
RemoveRedundant( std::vector<std::vector<vertex_t>>& v_in, const graph_t& g )
{
	std::vector<std::vector<vertex_t>> v_out;
	v_out.reserve( v_in.size() ); // to avoid unnecessary memory reallocations and copies

	std::set<VertexPair<vertex_t>> set_edges;

/// preliminary sorting by length, so we keep the shortest paths
	std::sort(
		std::begin(v_in),
		std::end(v_in),
		[]                                                                       // lambda
		( const std::vector<vertex_t>& vv1, const std::vector<vertex_t>& vv2 )
		{ return vv1.size() < vv2.size(); }
	);
	PrintPaths( std::cout, v_in, "After sorting" );

/// enumerate the cycles and store each edge in set. Add cycle to output only if new edge detected
    for( const auto& cycle: v_in )
    {
//		PrintPath( std::cout, cycle );
//		PrintSet( set_edges, "start" );
		bool newEdgeFound(false);
		for( size_t i=0; i<cycle.size(); ++i )
		{
			VertexPair<vertex_t> vp( (i==0?cycle[cycle.size()-1]:cycle[i-1]), cycle[i] );
			auto rv = set_edges.insert( vp );
			if( rv.second == true )  // if insertion took place, it means the edge wasn't already there
				newEdgeFound = true;
//			std::cout << "i=" << i << " vp=" << vp << " found=" << newEdgeFound << '\n';
		}
		if( newEdgeFound )
		{
//			std::cout << " => Adding current cycle to output vector\n";
			v_out.push_back( cycle );
		}
	}
	return v_out;
}
//-------------------------------------------------------------------------------------------
/// Builds the binary vector \c binvect associated to the cycle \c cycle.
/// The index map \c idx_map is used to fetch the index from the vertices
template<typename vertex_t>
void
BuildBinaryVector( const std::vector<vertex_t>& cycle, std::vector<bool>& binvect, const std::vector<size_t>& idx_map )
{
	for( size_t i=0; i<cycle.size(); ++i )
	{
		VertexPair<vertex_t> vp( (i==0?cycle[cycle.size()-1]:cycle[i-1]), cycle[i] );
		size_t idx1 = idx_map[vp.v1];
		size_t idx2 = idx1 + vp.v2 - 1;
//		std::cout << "vp: " << vp << " idx1=" << idx1 << " idx2=" << idx2 << std::endl;
		assert( idx2 < binvect.size() );
		binvect[idx2] = 1;
	}
//	PrintVector( std::cout, binvect );
}
//-------------------------------------------------------------------------------------------
/// builds all the binary vectors for all the cycles
template<typename vertex_t>
void
BuildBinaryVectors(
	const std::vector<std::vector<vertex_t>>& v_cycles,
	std::vector<std::vector<bool>>&           v_binvect,
	size_t                                    nbVertices )
{
	assert( v_cycles.size() == v_binvect.size() );

	size_t nbCombinations = nbVertices * (nbVertices-1) / 2;
	std::cout << "nbCombinations=" << nbCombinations << '\n';


/// build table of series $y_n = y_{n-1}+N-n-1$
	std::vector<size_t> idx_map( nbVertices-1 );
	idx_map[0] = 0;
	for( size_t i=1;i<nbVertices-1; i++ )
	{
		idx_map[i] = idx_map[i-1] + nbVertices - i - 1;
//		std::cout << "i=" << i << " map=" << idx_map[i] << '\n';
	}

	std::cout << "idx_map:\n";
	PrintVector( std::cout, idx_map );

    for( auto& binvect: v_binvect )
		binvect.resize( nbCombinations );

	for( size_t i=0; i<v_cycles.size(); i++ )
		BuildBinaryVector( v_cycles[i], v_binvect[i], idx_map );
}
//-------------------------------------------------------------------------------------------
/// Post-process step: removes paths (cycles) that are redundant (i.e. that can be deduced/build from the others)
/**
arg is not const, because it gets sorted here.
*/
template<typename vertex_t, typename graph_t>
std::vector<std::vector<vertex_t>>
RemoveRedundant2( std::vector<std::vector<vertex_t>>& v_in, const graph_t& g )
{
	std::vector<std::vector<vertex_t>> v_out;
	v_out.reserve( v_in.size() ); // to avoid unnecessary memory reallocations and copies

/// preliminary sorting by length, so we keep the shortest paths
	std::sort(
		std::begin(v_in),
		std::end(v_in),
		[]                                                                       // lambda
		( const std::vector<vertex_t>& vv1, const std::vector<vertex_t>& vv2 )
		{ return vv1.size() < vv2.size(); }
	);
	PrintPaths( std::cout, v_in, "After sorting" );

/// build for each cycle its associated binary vector

	size_t nbVertices = boost::num_vertices(g);
	std::vector<std::vector<bool>> v_binvect( v_in.size() );  // one binary vector per cycle
	BuildBinaryVectors( v_in, v_binvect, nbVertices );

/// build the composed cycle by taking each pair of cycle
/*    for( size_t i=0; i<v_in.size()-1; i++ )
		for( size_t j=i+1; i<v_in.size(); j++ )
		{
			std::cout << "i=" << i << " j=" << j << "\n";

		}
*/

	return v_out;

}
//-------------------------------------------------------------------------------------------
/// Cycle detector for an undirected graph
/**
Passed by value as visitor to \c boost::undirected_dfs()

See http://www.boost.org/doc/libs/1_58_0/libs/graph/doc/undirected_dfs.html
*/
template <typename vertex_t>
struct CycleDetector : public boost::dfs_visitor<>
{
	template<typename T1, typename T2>
	friend std::vector<std::vector<T2>> FindCycles( T1& g );

	public:
		CycleDetector()
		{
			v_source_vertex.clear();
		}
		bool cycleDetected() const { return !v_source_vertex.empty(); }
		template <class Edge, class Graph>
		void back_edge( Edge e, const Graph& g )     // is invoked on the back edges in the graph.
		{
			vertex_t vs = boost::source(e, g);
			vertex_t vt = boost::target(e, g);
	#ifdef UDGCD_PRINT_STEPS
			std::cout << " => CYCLE DETECTED! vs=" << vs << " vt=" << vt << "\n";
	#endif
			if(                                                                                                // add vertex to
				std::find( v_source_vertex.cbegin(), v_source_vertex.cend(), vs ) == v_source_vertex.cend()    // the starting point list
				&&                                                                                             // only if both are
				std::find( v_source_vertex.cbegin(), v_source_vertex.cend(), vt ) == v_source_vertex.cend()    // not already inside
			)
				v_source_vertex.push_back( vs );
		}
	private:
		static std::vector<vertex_t> v_source_vertex;
};

/// static var instanciation
template<class T>
std::vector<T> CycleDetector<T>::v_source_vertex;

//-------------------------------------------------------------------------------------------
/// Main user interface: just call this function to get the cycles inside your graph
/**
Returns a vector of cycles that have been found in the graph
*/
template<typename graph_t, typename vertex_t>
std::vector<std::vector<vertex_t>>
FindCycles( graph_t& g )
{
	if( boost::num_vertices(g) < 3 || boost::num_edges(g) < 3 )
		return std::vector<std::vector<vertex_t>>();

	CycleDetector<vertex_t> cycleDetector;

// vertex color map
	std::vector<boost::default_color_type> vertex_color( boost::num_vertices(g) );
	auto idmap = boost::get( boost::vertex_index, g );
	auto vcmap = make_iterator_property_map( vertex_color.begin(), idmap );

// edge color map
	std::map<typename graph_t::edge_descriptor, boost::default_color_type> edge_color;
	auto ecmap = boost::make_assoc_property_map( edge_color );

	boost::undirected_dfs( g, cycleDetector, vcmap, ecmap, 0 );

	if( !cycleDetector.cycleDetected() )             // if no detection,
		return std::vector<std::vector<vertex_t>>(); //  return empty vector, no cycles found


	std::vector<std::vector<vertex_t>> v_cycles;     // else, get the cycles.

#if 0

// find a vertex in each of the unconnected graphs
	std::vector<size_t> component( boost::num_vertices( g ) );
	auto nb_cc = boost::connected_components( g, &component[0] );
	std::cout << "nb_cc=" << nb_cc << " vect size=" << component.size() << '\n';

	std::vector<vertex_t> firstVertex( nb_cc );
	std::vector<bool>     firstVertexFlag( nb_cc, false );

	size_t nbFound = 0;
	for( size_t i=0; i != component.size() && nbFound != nb_cc; ++i )
	{
		std::cout << "Vertex " << i <<" is in component " << component[i] << '\n';
		if( firstVertexFlag[component[i]] == false )
		{
			firstVertex[component[i]] = i;
			firstVertexFlag[component[i]] = true;
			nbFound++;
		}
	}
    std::cout << '\n';

	for( const auto& vi: firstVertex )
	{
		std::cout << "considering vertex " << vi << ": v_cycle size=" << v_cycles.size() << '\n';
		std::vector<std::vector<vertex_t>> v_paths;
		std::vector<vertex_t> newv(1, vi ); // start by one of the filed source vertex
		v_paths.push_back( newv );
		explore( vi, g, v_paths, v_cycles );    // call of recursive function
	}

#else      // old way: search from all
	for( const auto& vi: cycleDetector.v_source_vertex )
	{
		std::vector<std::vector<vertex_t>> v_paths;
		std::vector<vertex_t> newv(1, vi ); // start by one of the filed source vertex
		v_paths.push_back( newv );
		explore( vi, g, v_paths, v_cycles );    // call of recursive function

#ifdef UDGCD_PRINT_STEPS
//	std::cout << "considering vertex " << vi << ": v_cycle size=" << v_cycles.size() << '\n';
//	PrintPaths( std::cout, v_cycles, "temp" );
#endif

	}
#endif

#ifdef UDGCD_PRINT_STEPS
	PrintPaths( std::cout, v_cycles, "Raw cycles" );
#endif

// post process 1: remove the paths that are identical but reversed
	std::vector<std::vector<vertex_t>> v_cycles2 = RemoveOppositePairs( v_cycles );
#ifdef UDGCD_PRINT_STEPS
	PrintPaths( std::cout, v_cycles2, "After removal of symmetrical cycles" );
#endif

// post process 2: remove twin paths
	std::vector<std::vector<vertex_t>> v_cycles3 = RemoveIdentical( v_cycles2 );
#ifdef UDGCD_PRINT_STEPS
	PrintPaths( std::cout, v_cycles3, "After removal of identical cycles" );
#endif

// post process 3: remove non-chordless cycles
	std::vector<std::vector<vertex_t>> v_cycles4 = RemoveNonChordless( v_cycles3, g );
#ifdef UDGCD_PRINT_STEPS
	PrintPaths( std::cout, v_cycles4, "After removal of non-chordless cycles" );
#endif

// post process 4:
	std::vector<std::vector<vertex_t>> v_cycles5 = RemoveRedundant2( v_cycles4, g );
#ifdef UDGCD_PRINT_STEPS
	PrintPaths( std::cout, v_cycles5, "After removal of composed cycles" );
#endif

	return v_cycles5;
}
//-------------------------------------------------------------------------------------------

} // namespace end

#endif // HG_UDGCD_HPP
