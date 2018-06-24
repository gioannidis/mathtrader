/* This file is part of MathTrader++.
 *
 * Copyright (C) 2018 George Ioannidis
 *
 * MathTrader++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MathTrader++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MathTrader++.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <fstream>
#include <ostream>
#include <string>

#ifndef _MATHTRADER_LIB_SOLVER_SRC_LEMON_IO_HPP_
#define _MATHTRADER_LIB_SOLVER_SRC_LEMON_IO_HPP_

/*! @brief LEMON Graph I/O Singleton.
 *
 *  A singleton class implementing useful input/output methods
 *  for LEMON-based graphs.
 *  so that it may be visualized by an appropriate third-party
 *  application.
 */
class LemonIo {
public:
	/*! @brief Get singleton instance.
	 *
	 *  Returns the only instance of the singleton class.
	 *  Guaranteed to be:
	 *  1. lazy initialized
	 *  2. destroyed correctly
	 */
	static LemonIo & getInstance() {
		static LemonIo instance;
		return instance;
	}

	/*! @brief Export graph to output stream as ``.dot``.
	 *
	 *  Exports the given LEMON graph to ``.dot`` format
	 *  to the given output stream.
	 *
	 *  @param[out]	os	output stream to export the graph to
	 *  @param[in]	graph	LEMON graph, inheriting from the ``Graph`` concept
	 *  @param[in]	node_label	LEMON node map with the corresponding node labels
	 *  @param[in]	title	a descriptive graph title
	 */
	template < typename DGR >
	static void exportToDot( std::ostream & os,
			const DGR & graph,
			const typename DGR::template NodeMap< std::string > & node_label,
			const std::string & title = ""
			);

	/*! @brief Export graph to file as ``.dot``.
	 *
	 *  Exports the given LEMON graph to ``.dot`` format
	 *  to the given file.
	 *  Calls @ref exportToDot.
	 *  If the output file exists, it is overwritten.
	 *
	 *  @param[in]	fn	output file to export the graph to
	 *  @param[in]	graph	LEMON graph, inheriting from the ``Graph`` concept
	 *  @param[in]	node_label	LEMON node map with the corresponding node labels
	 *  @param[in]	title	a descriptive graph title
	 */
	template < typename DGR >
	static void exportToDot( const std::string & file_name,
			const DGR & graph,
			const typename DGR::template NodeMap< std::string > & node_label,
			const std::string & title = ""
			);
private:
	/*! @brief Singleton constructor.
	 *
	 *  Constructs the singleton object.
	 *  Must be private, so no multiple objects may be instantiated.
	 */
	LemonIo() {}

	// Stop the compiler generating methods of copy the object
	LemonIo(const LemonIo & copy) = delete;	/* not Implemented */
	LemonIo & operator=(const LemonIo & copy) = delete;	/* not Implemented */
};

template < typename DGR >
void
LemonIo::exportToDot( std::ostream & os,
		const DGR & g,
		const typename DGR::template NodeMap< std::string > & node_label,
		const std::string & title ) {

	os << "digraph "
		<< title
		<< " {"
		<< std::endl;

	for ( typename DGR::NodeIt n(g); n != lemon::INVALID; ++n ) {
		os << "\t"
			<< "n" << g.id(n)
			<< " [label=\"" << node_label[n] << "\"];"
			<< std::endl;
	}
	for ( typename DGR::ArcIt a(g); a != lemon::INVALID; ++a ) {
		os << "\t"
			<< "n" << g.id( g.source(a) )
			<< " -> " << "n" << g.id(g.target(a))
			<< std::endl;
	}
	os << "}" << std::endl;
}

template < typename DGR >
void
LemonIo::exportToDot( const std::string & file_name,
		const DGR & g,
		const typename DGR::template NodeMap< std::string > & node_label,
		const std::string & title ) {

	std::filebuf fb;
	fb.open(file_name, std::ios::out);
	std::ostream os(&fb);
	try {
		exportToDot(os, g, node_label, title);
	} catch ( const std::exception & e ) {
		fb.close();
		throw;
	}
	fb.close();
}

#endif /* include guard */
