#pragma once
/*! \mainpage Easing Exchange Data Access

The Exchange Toolkit is a set of tools (in the ts3d namespace) that make common workflows easier, including
- [Tessellation based workflows](\ref example_obj)
 + Computing the net transform of a part
 + Computing the net visual style of a part
 + Obtaining an indexed triangle mesh of a part
- [Linking tessellation to the B-Rep](\ref example_compare_brep_tess)
- [Linking markup items to the B-Rep](\ref example_pmi_linked_items)


\section section_building Using the Toolkit

The toolkit is contained in header files only. To use the toolkit, simply add the desired headers to your project.
The source repository is hosted on GitHub [here](https://github.com/techsoft3d/ExchangeToolkit).

\section section_traversal Traversal

[Description](@ref traversal_page) | [API Reference](@ref traversal) 

The Exchange data model is a necessarily complex one. It captures macro and micro structures
from multi-file assemblies down to the geometry for individual parts. As a
result traversing the available data structures can be challenging.

To address this challenge the Exchange Toolkit provides functions for easily accessing
specific parts of the data model.

\section section_data_access Data Access

[Description](@ref access_page) | [API Reference](@ref access)

The general usage pattern employed by the Exchange API can be verbose. It requires you
to declare a struct, initialize it, use the "Get" API to fill the struct,
then to free you must call "Get" again with a null object pointer.

To ease the verbosity of this approach, Exchange Toolkit provides macros for easing data
access for most Exchange object types. Additionally, there are several functions for
common data access operations, such as querying the object's type and name.

\section section_eigen_bridge Eigen Bridge

[API Reference](@ref eigen_bridge)

The Exchange API contains transforms, matrices, vectors and positions. The API does
not address any mathematical operations on these objects.

The Exchange Toolkit provides a bridge to easily convert from Exchange objects to
the more standard [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page) toolkit.

\section section_examples Examples
Perhaps you learn best by [example](@ref examples)?


\page traversal_page Product Traversal

\tableofcontents

\section traversal_problem_statement The Challenge

The Exchange data model is complex. It captures details about a model file such as
[assembly organization](https://docs.techsoft3d.com/exchange/latest/build/product_occurrence.html),
[attribute inheritance](https://docs.techsoft3d.com/exchange/latest/build/managing_attribute_inheritance.html),
[PMI](https://docs.techsoft3d.com/exchange/latest/build/markup2.html),
[geometry and tessellation](https://docs.techsoft3d.com/exchange/latest/build/reading_geometry.html). As a
result, the code required to traverse the data model can be burdensome. Figure 1 illustrates
a portion of this complexity as a UML diagram with recursive containers. 

![Figure 1. UML diagram of a portion of the Exchange data model](ExchangeDataModel.png)

A direct approach for navigating the data structures to get to an object containing the 
information of interest typically involves writing one function per object type, with each 
implementation iterating over a child container and calling appropriate functions to traverse
deeper into the hierarchy. This has the downside of complexity and function implementations
that are just slight variations from one another, giving the feeling of code duplication. Another
possible implementation is to employ the 
[Visitor Design Pattern](https://en.wikipedia.org/wiki/Visitor_pattern). This approach has
a tendency of being overly complex and requires object inheritance, virtual function calls
and a bloated interface if it is to cover the entirety of the Exchange data model.

The Exchange Toolkit solves this problem in a much simpler way.

\section instance_path Concept: Instance Path

The Exchange Toolkit addresses this challenge by first introducing the concept of an 
ts3d::EntityArray. This is nothing more than a \c typedef
of a standard ordered container of \c A3DEntity*. An ordered container of generic
"base class" pointers allows us to represent a particular path through the model's hierarchy.
In the context of a path through the hierarchy, the alias ts3d::InstancePath is used.

\code
namespace ts3d {
	using EntityArray = std::vector<A3DEntity*>;
	using EntitySet = std::set<A3DEntity*>;
	using InstancePath = EntityArray;
	using InstancePathArray = std::vector<InstancePath>;
}
\endcode

\ref instance_path_details

\section leaf_node Concept: Leaf Node

Throughought the Exchange Toolkit, you may encounter the term "leaf node". This simply
refers to the _last_ object in an instance path.

\section traversal_functions Traversal Solution

The Exchange Toolkit provides just three functions for easily traversing the entirety of
the Exchange data model. They are:

-# ts3d::getLeafInstances( A3DEntity *owner, A3DEEntityType const &leaf_type )

	This function is used to obtain instance paths from the \c owner to all child
	objects of the type specified by \c leaf_type. The result is a ts3d::InstancePathArray
	containing unique paths. The leaf entity object may appear multiple times via differing paths.

	\note The type of the \c owner object is not important. The traversal algorithm will begin
	at whatever level of the hierarchy you need.

	As an example of how this can be used, imagine you'd like to know the total number of parts
	in a model file. This would look something like this:
	\code
		auto const all_parts = ts3d::getLeafInstances( loader.m_psModelFile, kA3DTypeAsmPartDefinition );
		std::cout << "This file contains " << all_parts.size() << " parts." << std::endl;
	\endcode
	This count would include all instances of shared parts.

	

-# ts3d::getUniqueLeafEntities( A3DEntity *owner, A3DEEntityType const &leaf_type );

	This function is used to obtain a set of unique child objects of type \c leaf_type. The result
	is a ts3d::EntitySet containing all unique leaf entities. This function does not provide any information
	about _how_ the hierarchy was traversed to arrive at these leaf objects.

	As an example of how this can be used, here is a snippet from the [attrib](@ref example_attrib) example. 
	In this code, \c brep_model is a ts3d::InstancePath containing a leaf object of type \c A3DRiBrepModel.
	\snippet attrib/main.cpp Getting all faces from an A3DRiBrepModel
	Internally, the B-Rep model is traversed and all \c A3DTopoFace objects are gathered and returned in 
	an unordered set. In this sample, we don't need to know the path to each \c A3DTopoFace in order to
	attach attributes, so this variation of the function is appropriate.


-# ts3d::getUniqueLeafEntities( A3DEntity *owner, A3DEEntityType const &leaf_type, InstancePathMap &instance_path_map )

	This overloaded variation can be used when you need a collection of unique child objects of a particular
	type _and_ you want the instance paths for each occurrence. The final parameter \c instance_path_map
	is used with each entity from the returned set as key values. The lookup returns an array of instance
	paths indicating the unique occurrences of the child object. The following code snippet from the 
	[bom](@ref example_bom)	example shows how to easily print a bill of materials.
	\snippet bom/main.cpp  Constructing a BOM
	Using <tt>samples/data/catiaV5/CV5_Micro_Engine/_micro engine.CATProduct</tt> as input, this code writes the
	following lines to standard out:
	\code{.txt}	
		"SCREW TOP": (4 instances)
		"CARBURETOR": (1 instance)
		"SCREW BACK": (4 instances)
		"HOUSING BACK": (1 instance)
		"BEARING PR UP": (1 instance)
		"PUSH ROD": (1 instance)
		"AXE": (1 instance)
		"BEARING PR DW": (1 instance)
		"CRANKSHAFT": (1 instance)
		"PISTON": (1 instance)
		"CYLINDER LINER": (1 instance)
		"HOUSING TOP": (1 instance)
		"HOUSING": (1 instance)
		"HOUSING FRONT": (1 instance)
		"BEARING CS": (1 instance)
	\endcode

\page instance_path_details Why is an InstancePath useful?
To understand the usefulness of this ordered collection, let's begin by examining a typical
product structure. Figure 2 shows a block diagram of the Exchange objects required to capture
assemblies of parts, as well as part instancing via the use of a prototype. This particular example
is from the micro engine CATIA assembly in the samples folder.

![Figure 2. Block diagram of a product hierarchy](ExchangeHierarchy.png)

The \c A3DAsmModelFile is the top level object. It contains a collection of
\c A3DAsmProductOccurrence objects. The \c A3DAsmProductOccurrence
object can contain children, a reference to a prototype, or an \c A3DAsmPartDefinition.
The part definition has a container of \c A3DRiRepresentationItem objects.

For visualization workflows, the representation item is the data structure that is the most
interesting because it contains the tessellation. In order to get to this data of interest
one must traverse (recursively) all of the aforementioned data structures. And, since a part
can be shared by multiple instances in the assembly, the specific traversal path is important.
The traversal path determines "net properties", such as location, color, visibility, etc.
Figure 3 shows a specific path through the hierarchy. A ts3d::InstancePath is the perfect 
container to represent this ordered collection of objects.

![Figure 3. Bold lines indicating a specific path to a representation item instance](InstancePath.png)


\page access_page Data Access
\tableofcontents
\section access_challenge The Challenge
If you are familiar with the Exchange API, then you have almost certainly seen this pattern before:
\code
// Assume you have some product occurrence and want to get its contents...
A3DAsmProductOccurrence *po = getSomePO();

// First you must declare a struct to hold data
A3DAsmProductOccurrenceData po_data;

// Second, you must initialize the struct so it can be used
A3D_INITIALIZE_DATA(A3DAsmProductOccurrenceData, po_data);

// Then you can retrieve the data
A3DAsmProductOccurrenceGet( po, &po_data );

// And now you can use the data
partProcessingAlgorithm( po_data.m_pPart );

// Finally, you must free any dynamic resources by calling
A3DAsmProductOccurrenceGet( nullptr, &po_data );
\endcode
Most programmers would agree that this is verbose and error prone.
\section access_data_wrapper Concept: Wrapper Macros
The Exchange Toolkit provides a solution to this pattern by exposing the macro A3D_HELPERS( A3D_VOID_TYPE ).
This macro is expanded for many of the Exchange data types which results in a set of structs that can be used
to read data from the Exchange API in a more concise way. 

In order to make the concept more clear let's take a look at the following code snippet, which is _similar_ to the code
expansion of the macro A3D_HELPERS( A3DAsmModelFile ).

\code
namespace ts3d {
	struct A3DAsmModelFileWrapper {
		// Acquire resources on creation
		A3DAsmModelFileWrapper( A3DAsmModelFile *p ) {
			A3D_INITIALIZE_DATA( A3DAsmModelFileData, _d );
			if( p ) {
				A3DAsmModelFileGet( p, &_d );
			}
		}

		// Free resources on destroy
		~A3DAsmModelFileWrapper( A3DAsmModelFile *p ) {
			A3DAsmModelFileGet( nullptr, &_d );
		}

		// Concise data access
		A3DAsmModelFileData *operator->( void ) {
			return &_d;
		}

		// Convenience function for reassignment
		void reset( A3DAsmModelFile *p ) {
			A3DAsmModelFileGet(nullptr, &_d );
			if( p ) {
				A3DAsmModelFileGet( p, &_d );
			}
		}

		A3DAsmModelFileData _d;
	};
}
\endcode

With this implementation, we now have easy access to the data from the Exchange API.
If we rewrite the \c doSomething code from above, it might look like this:
\code
// Assume you have some product occurrence and want to get its contents...
A3DAsmProductOccurrence *po = getSomePO();

// Declare the wrapper for concise access to the Exchange Data
ts3d::A3DAsmProductOccurrenceWrapper d( po );

// And now you can use the data
partProcessingAlgorithm( d->m_pPart );
\endcode

Or, if you want to go crazy with concise code, simply write:
\code
// A little ugly, but could be one line!
partProcessingAlgorithm( ts3d::A3DAsmProductOccurrentWrapper( getSomePO() )->m_pPart );
\endcode

\sa wrappers

\section instance_section Concept: Instance
The class ts3d::Instance provides additional functionality for computing "net attributes" that
are based on a specific ts3d::InstancePath. 

To understand this better, lets consider the specific example of a part that is instanced
multple times in a model. A bolt is a great example of this because one bolt size is often 
used throughout a model, with each instance having a different world position.

The world position of each bolt instance is determined by the nodes of the assembly tree that
contain it. Each node of the assembly tree imparts a local transformation, and once reaching
the leaf node of the product structure, you arrive at a part instance of the bolt with some
final accumulated transform.

Without the help of the Exchange Toolkit, one would have to write recursive code to traverse
the assembly hierarchy. At each node of the traversal, you must obtain the node's transform
and accumulate the transform with those previously encountered. To further complicate this
task, \c A3DAsmProductOccurrence objects store their transform in two different ways.

The ts3d::Instance class makes this task easier when used with the function 
ts3d::getNetMatrix(). Similarly, there are several attributes whose final (or "net")
values are determined by the path taken through the assembly hierarchy to arrive at a
particular leaf node.

Coupling this class with the Traversal functionality from exchange, you can easily obtain
net attributes from individual parts by writing the following code:
\code
auto const parts = ts3d::getLeafInstances( loader.m_psModelFile, kA3DTypeAsmPartDefinition );
for( auto const part_path : parts ) {
	ts3d::Instance part_instance( part_path );
	std::cout << "Net Matrix: " << ts3d::getNetMatrix( part_instance );
}
\endcode

\note ts3d::getNetMatrix() returns a ts3d::MatrixType, which is defined in the
\ref eigen_bridge.

\section Arrays

The Exchange API often contains C-style arrays. The struct containing the array will typically
have two data members, one for the pointer and another for the size of the array.

For convenience, the Exchange Toolkit provides the function ts3d::toVector for converting these arrays to
std::vector objects.

As an example, the traditional implementation:
\code
// Declare
A3DAsmProductOccurrenceData d;

// Initialize
A3D_INITIALIZE_DATA( A3DAsmProductOccurrenceData, d );

// Get
A3DAsmProductOccurrenceGet( po, &d );

// C-style Loop
for( auto idx = 0u; idx < d.m_uiPOccurrenceSize; ++idx ) {
	doSomethingWithChild( d.m_ppPOccurrences[idx] );
}

// Free
A3DAsmProductOccurrenceGet( nullptr, &d );
\endcode
Can be rewritten as:
\code
// Use the data access wrapper
ts3d::A3DAsmProductOccurrenceWrapper d( po );

// C++ style loop
for( auto const child_po : ts3d::toVector( d->m_ppPOccurrences, d->m_uiPOccurrencesSize ) ) {
	doSomethingWithChild( child_po );
}
\endcode

\section Tessellation
The Exchange API provides access to a tessellated representation of data in the form of
triangles, triangle strips and triangle fans. There are variations of the form this data
takes depending on the presence of per-vertex normal vectors, or texture coordinates.
This variation results in about 12 different cases for parsing the tessellation.

Many tessellation-based use cases are concerned with triangle primitives only. Fans and
strips must be decomposed, along with normal vectors and texture coordinates. This
simplification of the Exchange data set can lead to confusion and is indeed error prone.

To address this challenge, the Exchange Toolkit provides some tools and objects making
access to an index mesh much easier.

The class ts3d::RepresentationItemInstance is the entry point to obtaining the simplified
tessellation data. Since represenation items contain the tessellation, this class requires
a ts3d::InstancePath with a leaf node type that is a represenation item. Once constructed,
used the method ts3d::RepresentationItemInstance::getTessellation() to obtain the data.

\subsection Faces
The Exchange toolkit provides an abstraction that simplifies access to the triangles that
make up a topological face. The class that provides this functionality is ts3d::Tess3DInstance.
This is a concrete implementation of ts3d::TessBaseInstance returned from the call to
ts3d::RepresentationItemInstance::getTessellation(). From this object you can obtain a 
ts3d::TessFaceDataHelper object for each face.

Refer to this snippet of code extracted from [examples/obj/main.cpp](@ref example_obj) as an 
example of how this functionality can be used. This code writes the tessellation data to an
OBJ file. Note the use of ts3d::Instance::getNetShow() and ts3d::getNetMatrix().

\snippet obj/main.cpp Generate an OBJ file

\subsection Edges
ts3d::TessFaceDataHelper provides a method for obtaining the loops of edges associated with the
topological face it is associated with. ts3d::TessFaceDataHelper::loops() returns an ordered
collection of ts3d::TessFaceDataHelper::TessLoop objects. Each \c TessLoop contains an ordered
collection of ts3d::TessFaceDataHelper::TessEdge objects. Each \c TessEdge contains a visibility
flag, and an order collection of index values for the points of the polyline representing the
edge.

\sa \ref example_compare_brep_tess


\subsection Wires
\todo Implement and test wire tessellation objects
\note This implementation is incomplete!

\page examples Examples
\tableofcontents
All of the example code snippets found below are extracted from the \c examples folder.
\attention The sample code provided here and in the \c examples folder is provided as-is. You are
free to use it and adapt it as needed.

\section example_attrib Attaching attributes to B-Rep faces and edges
This snippet was extracted from <tt>examples/attrib/main.cpp</tt>.
\snippet attrib/main.cpp Attaching attributes to B-Rep faces and edges
\section example_bom Constructing a Bill of Materials
This snippet was extracted from <tt>examples/bom/main.cpp</tt>.
\snippet bom/main.cpp Constructing a BOM
\section example_compare_brep_tess Comparing tessellation to B-Rep
This snippet was extracted from <tt>examples/compare_brep_tess/main.cpp</tt>.
\snippet compare_brep_tess/main.cpp Compare tessellation to b-rep
\section example_pmi_linked_items How to access Markup/PMI Linked Items
This snippet was extracted from <tt>examples/pmi_linked_items/main.cpp</tt>.
\snippet pmi_linked_items/main.cpp Dump markup linked items
\section example_wire_body_curve_types Print curve types that make up a wire body
This snippet was extract from <tt>examples/wire_body_curve_types/main.cpp</tt>.
\snippet wire_body_curve_types/main.cpp Print wire body curve types
\section example_obj Write an OBJ file
This snippet was extract from <tt>examples/obj/main.cpp</tt>.
\snippet obj/main.cpp Generate an OBJ file
\section example_print_structure Print the product structure
These snippers were extracted from <tt>examples/print_structure/main.cpp</tt>
\snippet print_structure/main.cpp Print object name and recurse
This function is used in the body of main as follows.
\snippet print_structure/main.cpp Load model and print structure

\defgroup traversal Product Traversal
\brief Easily obtain child nodes in the Exchange data model based on object type.
\sa \ref traversal_page

\defgroup access Data Access
\brief More easily access data inside the Exchange data model.
\sa \ref access_page

\defgroup wrappers Data Access Wrappers
\brief This is a complete list of data access wrappers. Every object in HOOPS Exchange
with a typename matching A3D[a-zA-Z0-9]+Data that has a corresponding A3D[a-zA-Z0-9]+Get
has a wrapper entry below.
\ingroup access

\defgroup eigen_bridge Eigen Bridge
\brief Translate Exchange objects into Eigen objects to make matrix math easier and more reliable.

\namespace ts3d
\brief The ts3d namespace is used to contain all Exchange Toolkit functionality.

*/

#include <vector>
#include <set>
#include <unordered_map>
#include <functional>
#include <memory>
#include <algorithm>
#include <iterator>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4503)
#endif

namespace ts3d {
    /*!
     \class CheckResult
     \brief This class is used to log calls to the Exchange API.
     
     Information about each call is recorded when you use the macro
     \c CheckResult
     */
    class CheckResult {
    public:
        /*! \brief Gets the global instances of the CheckResult object.
         The returned object can be used to examine information about the
         most recent call to an Exchange API invoked by the \c CheckResult
         macro.
         */
        static CheckResult &instance( void ) {
            static CheckResult _instance;
            return _instance;
        }

        /*! \brief Sets the callback function that is invoked when an call is made
         and the result value is not \c A3D_SUCCESS. The default implementation
         writes the function, file, line and status codes to std::cerr.
         */
        void setFailureCallback( std::function<bool(CheckResult const&)> failure_cb ) {
            _failureCallback = failure_cb;
        }
        
        /*! \brief Gets the filename where the last call was invoked from.
         */
        std::string const &file( void ) const {
            return _file;
        }
        
        /*! \brief Gets the line number where the last call was invoked from.
         */
        int const &line( void ) const {
            return _line;
        }
        
        /*! \brief Gets the stringified representation of the last function call.
         */
        std::string const &fn( void ) const {
            return _fn;
        }
        
        /*! \brief Gets the return code resulting from the last function call.
         */
        A3DStatus const &result( void ) const {
            return _result;
        }
        
        
        /*! \brief Used internally by the \c CheckResult macro to log a function call.
         \internal
         */
        bool _record( std::string const file, int const line_no, std::string const fn, A3DStatus const result ) {
            _file = file;
            _line = line_no;
            _fn = fn;
            _result = result;
            if( _result != A3D_SUCCESS && _failureCallback ) {
                return _failureCallback(*this);
            }
            return A3D_SUCCESS == _result;
        }
        
    private:
        CheckResult( void ) {}
        CheckResult( CheckResult const &other ) = delete;
        
        std::string _file;
        int _line = 0;
        std::string _fn;
        A3DStatus _result = A3D_SUCCESS;
        std::function<bool(CheckResult const&)> _failureCallback = [](ts3d::CheckResult const &r ) {
            std::string const error_string = A3DMiscGetErrorMsg( r.result() );
            std::cerr << "API FAILURE: " << r.fn() << " [" << r.file() << ":" << r.line() << "] == " << error_string << "(" << r.result() << ")" << std::endl;
            return false;
        };

    };
}

/*!
 \brief If you wrap all of your Exchange API calls in this macro,
 information about the call is recorded in the object \c ts3d::CheckResult::instance().
 You can configure a default handler for when the result is not equal to A3D_SUCCESS.
 */
#define CheckResult( f ) \
    ts3d::CheckResult::instance()._record( __FILE__, __LINE__, #f, f )

/*! 
 * \brief This macro is used to define a struct that can be used to
 * more easily obtain and access the Exchange data object's member
 * variables. 
 * \sa For a documented interface of this macro expansion, see the
 * sample expansion ts3d::A3DRootBaseWrapper.
 * \ingroup access
 */

#define A3D_HELPERS( A3D_VOID_TYPE ) \
namespace ts3d {\
/*! \struct A3D_VOID_TYPE ## Wrapper */ \
/*! \ingroup wrappers */ \
/*! \brief Provides a wrapper for accessing \c A3D_VOID_TYPE ## Data. */ \
struct A3D_VOID_TYPE ## Wrapper { \
    \
    /*! \brief Alias for the data type */ \
    using DataType = A3D_VOID_TYPE ## Data; \
    \
    /*! \brief Constructs a wrapper, initializing the data storage with values obtained using the provided pointer and \c A3DRootBaseGet. */\
    /*!  This can be a \c nullptr, in which case the \c A3DRootBaseData struct is initialized but not populated. */ \
    A3D_VOID_TYPE ## Wrapper( A3D_VOID_TYPE *ntt = nullptr ) { \
        A3D_INITIALIZE_DATA( A3D_VOID_TYPE ## Data, _d ); \
        if( ntt ) A3D_VOID_TYPE ## Get( ntt, &_d ); \
    } \
    \
    /*! \brief The destructor is implemented to call <tt>A3D_VOID_TYPE ## Get( nullptr, &_d )</tt> to ensure dynamic resources are freed. */ \
    ~A3D_VOID_TYPE ## Wrapper( void ) { \
        A3D_VOID_TYPE ## Get( nullptr, &_d ); \
    } \
    \
    /*! \brief Const data access to \c A3D_VOID_TYPE ## Data. */ \
    DataType const *operator->() const { \
        return &_d; \
    } \
    \
    /*! \brief Non-const data access to \c A3D_VOID_TYPE ## Data. */ \
    /*! \note If you modify the data and expect the underlying Exchange data model to be updated, you must explicitly call the correct \c Create or \c Set function in the Exchange API. This wrapper does not provide write capability. */ \
    /*! \todo Implement write capability? */ \
    DataType *operator->() { \
        return &_d; \
    } \
    \
    /*!  \brief Resets internal data to values obtained from the provided pointer. */ \
    /*!  Calling this function will free dynamic resources as needed prior to obtaining new values in the struct.
    If a \c nullptr is provided, the data is not explicitly changed and may reflect old values. */ \
    void reset( A3D_VOID_TYPE *ntt ) { \
        A3D_VOID_TYPE ## Get( nullptr, &_d ); \
        if( ntt ) A3D_VOID_TYPE ## Get( ntt, &_d ); \
    }\
    \
    /*!  \brief Data storage */ \
    DataType _d; \
    }; \
}

A3D_HELPERS(A3DAsmFilter)
A3D_HELPERS(A3DAsmModelFile)
A3D_HELPERS(A3DAsmPartDefinition)
A3D_HELPERS(A3DAsmProductOccurrence)
A3D_HELPERS(A3DCrvBlend02Boundary)
A3D_HELPERS(A3DCrvCircle)
A3D_HELPERS(A3DCrvComposite)
A3D_HELPERS(A3DCrvEllipse)
A3D_HELPERS(A3DCrvEquation)
A3D_HELPERS(A3DCrvHelix)
A3D_HELPERS(A3DCrvHyperbola)
A3D_HELPERS(A3DCrvIntersection)
A3D_HELPERS(A3DCrvLine)
A3D_HELPERS(A3DCrvNurbs)
A3D_HELPERS(A3DCrvOffset)
A3D_HELPERS(A3DCrvOnSurf)
A3D_HELPERS(A3DCrvParabola)
A3D_HELPERS(A3DCrvPolyLine)
A3D_HELPERS(A3DCrvTransform)
A3D_HELPERS(A3DDrawingBlockBasic)
A3D_HELPERS(A3DDrawingBlockOperator)
A3D_HELPERS(A3DDrawingClipFrame)
A3D_HELPERS(A3DDrawingCurve)
A3D_HELPERS(A3DDrawingFilledArea)
A3D_HELPERS(A3DDrawingModel)
A3D_HELPERS(A3DDrawingPicture)
A3D_HELPERS(A3DDrawingSheet)
A3D_HELPERS(A3DDrawingSheetFormat)
A3D_HELPERS(A3DDrawingVertices)
A3D_HELPERS(A3DDrawingView)
A3D_HELPERS(A3DFRMFeature)
A3D_HELPERS(A3DFRMFeatureLinkedItem)
A3D_HELPERS(A3DFRMFeatureTree)
A3D_HELPERS(A3DFRMParameter)
A3D_HELPERS(A3DGlobal)
A3D_HELPERS(A3DGraphAmbientLight)
A3D_HELPERS(A3DGraphCamera)
A3D_HELPERS(A3DGraphDirectionalLight)
A3D_HELPERS(A3DGraphPointLight)
A3D_HELPERS(A3DGraphSceneDisplayParameters)
A3D_HELPERS(A3DGraphSpotLight)
A3D_HELPERS(A3DGraphTextureTransformation)
A3D_HELPERS(A3DGraphics)
A3D_HELPERS(A3DHLRRepresentationItem)
A3D_HELPERS(A3DMDDimensionCombinedToleranceFormat)
A3D_HELPERS(A3DMDDimensionExtentionLine)
A3D_HELPERS(A3DMDDimensionExtremity)
A3D_HELPERS(A3DMDDimensionForeshortened)
A3D_HELPERS(A3DMDDimensionFunnel)
A3D_HELPERS(A3DMDDimensionLine)
A3D_HELPERS(A3DMDDimensionLineSymbol)
A3D_HELPERS(A3DMDDimensionSecondPart)
A3D_HELPERS(A3DMDDimensionSimpleToleranceFormat)
A3D_HELPERS(A3DMDDimensionValue)
A3D_HELPERS(A3DMDDimensionValueFormat)
A3D_HELPERS(A3DMDFCFDraftingRow)
A3D_HELPERS(A3DMDFCFDrawingRow)
A3D_HELPERS(A3DMDFCFIndicator)
A3D_HELPERS(A3DMDFCFRowDatum)
A3D_HELPERS(A3DMDFCFToleranceValue)
A3D_HELPERS(A3DMDFCProjectedZone)
A3D_HELPERS(A3DMDFCTolerancePerUnit)
A3D_HELPERS(A3DMDFCValue)
A3D_HELPERS(A3DMDFeatureControlFrame)
A3D_HELPERS(A3DMDLeaderDefinition)
A3D_HELPERS(A3DMDLeaderSymbol)
A3D_HELPERS(A3DMDMarkupLeaderStub)
A3D_HELPERS(A3DMDPosition2D)
A3D_HELPERS(A3DMDPosition3D)
A3D_HELPERS(A3DMDPositionReference)
A3D_HELPERS(A3DMDTextPosition)
A3D_HELPERS(A3DMDTextProperties)
A3D_HELPERS(A3DMDToleranceSize)
A3D_HELPERS(A3DMDToleranceSizeValue)
A3D_HELPERS(A3DMarkupBalloon)
A3D_HELPERS(A3DMarkupCoordinate)
A3D_HELPERS(A3DMarkupDatum)
A3D_HELPERS(A3DMarkupDefinition)
A3D_HELPERS(A3DMarkupDimension)
A3D_HELPERS(A3DMarkupFastener)
A3D_HELPERS(A3DMarkupGDT)
A3D_HELPERS(A3DMarkupLineWelding)
A3D_HELPERS(A3DMarkupLocator)
A3D_HELPERS(A3DMarkupMeasurementPoint)
A3D_HELPERS(A3DMarkupRichText)
A3D_HELPERS(A3DMarkupRoughness)
A3D_HELPERS(A3DMarkupSpotWelding)
A3D_HELPERS(A3DMarkupText)
A3D_HELPERS(A3DMathFct1DArctanCos)
A3D_HELPERS(A3DMathFct1DCombination)
A3D_HELPERS(A3DMathFct1DFraction)
A3D_HELPERS(A3DMathFct1DPolynom)
A3D_HELPERS(A3DMathFct1DTrigonometric)
A3D_HELPERS(A3DMathFct3DLinear)
A3D_HELPERS(A3DMathFct3DNonLinear)
A3D_HELPERS(A3DMiscAttribute)
A3D_HELPERS(A3DMiscCartesianTransformation)
A3D_HELPERS(A3DMiscCascadedAttributes)
A3D_HELPERS(A3DMiscEntityReference)
A3D_HELPERS(A3DMiscGeneralTransformation)
A3D_HELPERS(A3DMiscMarkupLinkedItem)
A3D_HELPERS(A3DMiscReferenceOnCsysItem)
A3D_HELPERS(A3DMiscReferenceOnTess)
A3D_HELPERS(A3DMiscReferenceOnTopology)
A3D_HELPERS(A3DMkpAnnotationItem)
A3D_HELPERS(A3DMkpAnnotationReference)
A3D_HELPERS(A3DMkpAnnotationSet)
A3D_HELPERS(A3DMkpLeader)
A3D_HELPERS(A3DMkpMarkup)
A3D_HELPERS(A3DMkpRTFField)
A3D_HELPERS(A3DMkpView)
A3D_HELPERS(A3DRiBrepModel)
A3D_HELPERS(A3DRiCoordinateSystem)
A3D_HELPERS(A3DRiCurve)
A3D_HELPERS(A3DRiDirection)
A3D_HELPERS(A3DRiPlane)
A3D_HELPERS(A3DRiPointSet)
A3D_HELPERS(A3DRiPolyBrepModel)
A3D_HELPERS(A3DRiPolyWire)
A3D_HELPERS(A3DRiRepresentationItem)
A3D_HELPERS(A3DRiSet)
A3D_HELPERS(A3DRootBase)
A3D_HELPERS(A3DRootBaseWithGraphics)
A3D_HELPERS(A3DSurfBlend01)
A3D_HELPERS(A3DSurfBlend02)
A3D_HELPERS(A3DSurfBlend03)
A3D_HELPERS(A3DSurfCone)
A3D_HELPERS(A3DSurfCylinder)
A3D_HELPERS(A3DSurfCylindrical)
A3D_HELPERS(A3DSurfExtrusion)
A3D_HELPERS(A3DSurfFromCurves)
A3D_HELPERS(A3DSurfNurbs)
A3D_HELPERS(A3DSurfOffset)
A3D_HELPERS(A3DSurfPipe)
A3D_HELPERS(A3DSurfPlane)
A3D_HELPERS(A3DSurfRevolution)
A3D_HELPERS(A3DSurfRuled)
A3D_HELPERS(A3DSurfSphere)
A3D_HELPERS(A3DSurfTorus)
A3D_HELPERS(A3DSurfTransform)
A3D_HELPERS(A3DTess3D)
A3D_HELPERS(A3DTess3DWire)
A3D_HELPERS(A3DTessBase)
A3D_HELPERS(A3DTessMarkup)
A3D_HELPERS(A3DTopoBody)
A3D_HELPERS(A3DTopoBrepData)
A3D_HELPERS(A3DTopoCoEdge)
A3D_HELPERS(A3DTopoConnex)
A3D_HELPERS(A3DTopoContext)
A3D_HELPERS(A3DTopoEdge)
A3D_HELPERS(A3DTopoFace)
A3D_HELPERS(A3DTopoLoop)
A3D_HELPERS(A3DTopoMultipleVertex)
A3D_HELPERS(A3DTopoShell)
A3D_HELPERS(A3DTopoSingleWireBody)
A3D_HELPERS(A3DTopoUniqueVertex)
A3D_HELPERS(A3DTopoWireEdge)



namespace ts3d {
    /*! \brief This function can be used to more easily construct
     * a vector of objects.
     * \ingroup access
     */
    template<typename T, typename S>
    std::vector<T> toVector( T *d, S const &sz ) {
        return std::vector<T>( d, d+sz );
    }
}

namespace {
    std::pair<A3DBool, double> getUnitFromPO( A3DAsmProductOccurrence *po ) {
        if( nullptr == po ) {
            return std::make_pair(static_cast<A3DBool>( false ), 0. );
        }
    
        ts3d::A3DAsmProductOccurrenceWrapper d( po );
        
        if(!d->m_bUnitFromCAD && nullptr != d->m_pPrototype ) {
            return getUnitFromPO( d->m_pPrototype );
        }
        
        return std::make_pair( d->m_bUnitFromCAD, d->m_dUnit );
    }
    
#pragma warning(push)
#pragma warning(disable: 4702)    
    double getUnitFactor( std::vector<A3DAsmProductOccurrence*> const &pos ) {
        for( auto po : pos ) {
            auto po_unit = getUnitFromPO( po );
            if(po_unit.first) {
                return po_unit.second;
            } else {
                ts3d::A3DAsmProductOccurrenceWrapper d( po );
                auto const child_pos = ts3d::toVector( d->m_ppPOccurrences, d->m_uiPOccurrencesSize );
                return getUnitFactor( child_pos );
            }
        }
        return -1.;
    }
#pragma warning(pop)
}

namespace ts3d {
    /*! \brief Used to abstract an ordered collection of Exchange objects.
     *  \ingroup access
     */
    using EntityArray = std::vector<A3DEntity*>;

    /*! \brief Used to absract an unordered collection of unique Exchange objects.
     *  \ingroup access
     */
    using EntitySet = std::set<A3DEntity*>;
    
    /*! \brief Check if type is Ri or derived Ri type
     *  \ingroup access
     */
    static inline bool isRepresentationItem( A3DEEntityType const &t ) {
        return (kA3DTypeRiRepresentationItem == t ||
                kA3DTypeRiSet == t ||
                kA3DTypeRiCurve == t ||
                kA3DTypeRiPlane == t ||
                kA3DTypeRiPointSet == t ||
                kA3DTypeRiPolyWire == t ||
                kA3DTypeRiPolyBrepModel == t ||
                kA3DTypeRiBrepModel == t ||
                kA3DTypeRiDirection == t);
    }

    /*! \brief Check if type is TessBase or derived TessBase type
     *  \ingroup access
     */
    static inline bool isTessBase( A3DEEntityType const &t ) {
        return (kA3DTypeTessBase == t ||
                kA3DTypeTess3D == t ||
                kA3DTypeTess3DWire == t ||
                kA3DTypeTessMarkup == t);
    }

    /*! \brief Check if type is A3DMkpAnnotationEntity or derived type
        \ingroup access
     */
    static inline bool isAnnotationEntity( A3DEEntityType const &t ) {
        return (kA3DTypeMkpAnnotationItem == t ||
                kA3DTypeMkpAnnotationSet == t ||
                kA3DTypeMkpAnnotationReference == t);
    }

    /*! \brief Check if type is A3DMkpMarkup or derived type
     \ingroup access
     */
    static inline bool isMarkup( A3DEEntityType const &t ) {
        return (kA3DTypeMDPosition2D == t ||
                kA3DTypeMDPosition3D == t ||
                kA3DTypeMDPositionReference == t ||
                kA3DTypeMarkupCoordinate == t ||
                kA3DTypeMarkupText == t ||
                kA3DTypeMDTextPosition == t ||
                kA3DTypeMarkupRichText == t ||
                kA3DTypeMarkupDimension == t ||
                kA3DTypeMarkupDatum == t ||
                kA3DTypeMarkupGDT == t ||
                kA3DTypeMarkupRoughness == t ||
                kA3DTypeMarkupBalloon == t ||
                kA3DTypeMarkupFastener == t ||
                kA3DTypeMarkupLocator == t ||
                kA3DTypeMarkupMeasurementPoint == t ||
                kA3DTypeMarkupLineWelding == t ||
                kA3DTypeMarkupSpotWelding == t);
    }

    /*! \brief A simple wrapper to allow use inline without having to
     * declare a temporary variable to the return value.
     *  \ingroup access
     */
    static inline A3DEEntityType getEntityType( A3DEntity *ntt ) {
        auto result = kA3DTypeUnknown;
        if( nullptr != ntt ) {
            A3DEntityGetType( ntt, &result );
        }
        return result;
    }
    
    /*! \brief Can be used by functions offering an option to obtain
     *  an attribute from a product occurrence or its prototype.
     *  \ingroup access
     *  \private
     */
    enum class PrototypeOption {
        /*! \brief Consider the prototype if attribute doesn't exist. */
        Use,
        /*! \brief Do not consider the prototype if attribute doesn't exist. */
        DoNotUse
    };

    /*! \brief Gets a part definition from a product occurrence, optionally
     * using recursion to query the prototype.
     * \internal
     */
    static inline A3DAsmPartDefinition *getPartDefinition( A3DAsmProductOccurrence *po, PrototypeOption const &opt = PrototypeOption::Use ) {
        if( nullptr == po ) {
            return nullptr;
        }
        A3DAsmProductOccurrenceWrapper d( po );
        return d->m_pPart ? d->m_pPart : (opt == PrototypeOption::Use ? getPartDefinition( d->m_pPrototype, opt ) : nullptr );
    }

    /*! \brief Gets the child product occurrences from a parent, optionally using recursion to query the prototype.
     \internal
     */
    static inline EntityArray getProductOccurrences( A3DAsmProductOccurrence *po, PrototypeOption const &opt = PrototypeOption::Use ) {
        if( nullptr == po ) {
            return EntityArray();
        }
        
        A3DAsmProductOccurrenceWrapper d( po );
        return d->m_uiPOccurrencesSize != 0 ? toVector( d->m_ppPOccurrences, d->m_uiPOccurrencesSize ) : (opt == PrototypeOption::Use ? getProductOccurrences( d->m_pPrototype, opt ) : EntityArray() );
    }

    /*! \brief Obtains the unit scaling factor (units/mm)
     *  for a given input model file.
     *  \ingroup access
     */
    static inline double getUnit( A3DAsmModelFile *modelFile ) {
        A3DAsmModelFileWrapper d( modelFile );
        return d->m_bUnitFromCAD ? d->m_dUnit : getUnitFactor( toVector( d->m_ppPOccurrences, d->m_uiPOccurrencesSize ) );
    }
}

/*! \brief This is used to indicate _all_ vertex types
 *  \internal */
#define kA3DTypeTopoVertex static_cast<A3DEEntityType>(kA3DTypeTopo + 18)

/*! \brief This is used to indicate _all_ markup types
 *  \internal */
#define kA3DTypeMkpAnnotationEntity static_cast<A3DEEntityType>(kA3DTypeMkp + 7)



namespace ts3d {
    /*!
     * \brief The InstancePath type is used to identify a particular path through the
     * hierarchy of Exchange objects to reach a particular entity. 
     * The path to an entity is important because it provides additional context for how
     * attributes and behaviors of the leaf nodes should be presented.
     * EntityArray is an alias for a standard container of A3DEntity* values.
     * \ingroup traversal
     */
    using InstancePath = EntityArray;

    
    /*! \brief Used as a collection of InstancePath objects. When you ask for
     * all leaf nodes of a particular leaf node type, the result is stored in
     * an InstancePathArray. See getLeafInstances
     * \ingroup traversal
     */
    using InstancePathArray = std::vector<InstancePath>;

    /*! \brief Returns an array of instance paths, each with a leaf entity type
     * corresponding to \c leaf_type. 
     * 
     * The top level object provided by the parameter
     * \c owner is traversed (recursively as needed) to obtain all leaf instances.
     * Each instance path in the returned instance path array will be unique, however
     * there may be multiple paths that lead to the same object. This occurs when
     * a part is shared by multiple assemblies. If your workflow requires unique
     * leaf entities (such as parts) and an array of instances paths that lead to each child, use the
     * function getUniqueLeafEntities() instead.
     * \param owner The top level object to begin traversal.
     * \param leaf_type The type of child node to look for.
     * \return A collection of instance path objects which define a specific path.
     * through the object hierarchy, beginning with the specified \c owner. The final
     * object in each array will be of type \c leaf_type.
     * \ingroup traversal
     */
    static InstancePathArray getLeafInstances( A3DEntity *owner, A3DEEntityType const &leaf_type );

    /*! \private */
    static inline EntitySet getUniqueParts( A3DAsmModelFile *modelFile );

    /*! \brief Used for looking up instance path for a given unique child.
     * \ingroup traversal
     */
    using InstancePathMap = std::unordered_map<A3DEntity*, InstancePathArray>;

    /*! \brief Returns a set of \c A3DEntity pointers that are unique leaf entities
     * of type leaf_type of the provided owner. 
     * \param owner A pointer to a top level object which will be traversed.
     * \param leaf_type The type of child object that should be searched for.
     * \return A set of all unique leaf entities of type \c leaf_type.
     * \ingroup traversal
     */
    static inline EntitySet getUniqueLeafEntities( A3DEntity *owner, A3DEEntityType const &leaf_type );

    /*! \deprecated Please use getUniqueLeafEntities instead.
     */
    static inline EntitySet getUniqueChildren( A3DEntity *owner, A3DEEntityType const &leaf_type ) {
        return getUniqueLeafEntities( owner, leaf_type );
    }

    /*! \brief Returns a set of \c A3DEntity pointers that are unique leaf entities and all the instance paths referencing each child.
     * \param owner A pointer to a top level object which will be traversed.
     * \param leaf_type The type of child object that should be searched for.
     * \param instance_path_map A map containing the collection for all paths referencing each unique child occurrence.
     * \return A set of all unique leaf entities of type \c leaf_type. The values from this set should be used to
     * obtain the referencing paths from the \c instance_path_map.
     * \ingroup traversal
     */
    static inline EntitySet getUniqueLeafEntities( A3DEntity *owner, A3DEEntityType const &leaf_type, InstancePathMap &instance_path_map );

    /*! \deprecated Please use getUniqueLeafEntities instead
     */
    static inline EntitySet getUniqueChildren( A3DEntity *owner, A3DEEntityType const &leaf_type, InstancePathMap &instance_path_map ) {
        return getUniqueLeafEntities( owner, leaf_type, instance_path_map );
    }

    /*! \brief Easily obtain a subset of an InstancePath with a final node of a specific type.
     *  \param instance_path The instance path to obtain the parent for
     *  \param owner_type The type of owner to search for
     *  \return An instance path whose last object is of type \c owner_type. Or empty if to owning type is found.
     *  \ingroup traversal
     */
    static inline InstancePath getOwningInstance( InstancePath const &instance_path, A3DEEntityType const &owner_type );

    /*! \brief Gets all immediate children (if any) of a specified type from a parent object
     \param parent Object that will be queried to obtain children
     \param child_type The types of the child objects desired
     \return An ordered collection of child objects of type \c child_type
     \ingroup traversal
     */
    static inline EntityArray getChildren( A3DEntity *parent, A3DEEntityType const &child_type );

}


namespace std {
    /*! \private */
    template <> struct hash<A3DEEntityType> {
        size_t operator()(A3DEEntityType const& s) const noexcept {
            return s;
        }
    };
    template<>  struct hash<ts3d::InstancePath> {
        size_t operator()(ts3d::InstancePath const &i ) const noexcept {
            auto seed = i.size();
            auto const magic_number = reinterpret_cast<size_t>( A3DAsmModelFileGet );
            for( auto &ntt : i ) {
                seed ^= reinterpret_cast<size_t>( ntt ) + magic_number + (seed << 6) + (seed >>2 );
            }
            return seed;
        }
    };
}

namespace {
    using namespace ts3d;
    using TypeSet = std::set<A3DEEntityType>;
    using TypePath = std::vector<A3DEEntityType>;
    using TypePathArray = std::vector<TypePath>;

    // Function object that can be invoked to obtain an EntityArray
    // of the child objects
    using GetterFunction = std::function<EntityArray (A3DEntity*)>;

    // The GetterMap is used to look up a getter function for a
    // particular child object type
    using GetterMap = std::unordered_map<A3DEEntityType, GetterFunction>;

    // The GetterMapsByType is used to look up a GetterMap for a
    // particular owning object type
    using GetterMapsByType = std::unordered_map<A3DEEntityType, GetterMap>;
    
    // This is the static instance of a GetterMapsByType. It is
    // initialized with parent type to { child type } hash, and
    // for each child type it contains a function object for
    // obtaining the children of that type. See the typedef
    // statements above to better understand the initializer
    // syntax.
    static GetterMapsByType _getterMapByType = {
        {   // an entry in GetterMapsByType
            kA3DTypeAsmModelFile, // A3DEEntityType (owning object type)
            { // GetterMap
                { // an entry in GetterMap
                    kA3DTypeAsmProductOccurrence, // A3DEEntityType (child object type)
                    [](A3DEntity *ntt) {          // GetteFunction (function for getting children)
                        A3DAsmModelFileWrapper d( ntt );
                        return toVector( d->m_ppPOccurrences, d->m_uiPOccurrencesSize );
                    }
                }
            }
        },
        {
            kA3DTypeAsmProductOccurrence,
            {
                {
                    kA3DTypeAsmProductOccurrence,
                    [](A3DEntity *ntt) {
                        return getProductOccurrences( ntt, PrototypeOption::Use );
                    }
                },
                {
                    kA3DTypeAsmPartDefinition,
                    [](A3DEntity *ntt) {
                        auto const part_definition = getPartDefinition( ntt, PrototypeOption::Use );
                        return nullptr == part_definition ? EntityArray() : EntityArray( 1, part_definition );
                    }
                },
                {
                    kA3DTypeMkpView,
                    [](A3DEntity *ntt) {
                        A3DAsmProductOccurrenceWrapper d( ntt );
                        return toVector( d->m_ppViews, d->m_uiViewsSize );
                    }
                },
                {
                    kA3DTypeGraphCamera,
                    [](A3DEntity *ntt) {
                        A3DAsmProductOccurrenceWrapper d( ntt );
                        return toVector( d->m_ppCamera, d->m_uiCameraSize );
                    }
                },
                {
                    kA3DTypeMkpAnnotationEntity,
                    [](A3DEntity *ntt) {
                        A3DAsmProductOccurrenceWrapper d( ntt );
                        return toVector( d->m_ppAnnotations, d->m_uiAnnotationsSize );
                    }
                }
            }
        },
        {
            kA3DTypeAsmPartDefinition,
            {
                {
                    kA3DTypeRiRepresentationItem,
                    [](A3DEntity *ntt) {
                        A3DAsmPartDefinitionWrapper d( ntt );
                        return toVector( d->m_ppRepItems, d->m_uiRepItemsSize );
                    }
                },
                {
                    kA3DTypeMkpAnnotationEntity,
                    [](A3DEntity *ntt) {
                        A3DAsmPartDefinitionWrapper d( ntt );
                        return toVector( d->m_ppAnnotations, d->m_uiAnnotationsSize );
                    }
                },
                {
                    kA3DTypeMkpView,
                    []( A3DEntity *ntt) {
                        A3DAsmPartDefinitionWrapper d( ntt );
                        return toVector( d->m_ppViews, d->m_uiViewsSize );
                    }
                }
            }
        },
        {
            kA3DTypeRiRepresentationItem,
            {
                {
                    kA3DTypeRiRepresentationItem,
                    [](A3DEntity *ntt) {
                        if( kA3DTypeRiSet == getEntityType( ntt ) ) {
                            A3DRiSetWrapper d( ntt );
                            return toVector( d->m_ppRepItems, d->m_uiRepItemsSize );
                        }
                        return EntityArray();
                    }
                },
                {
                    kA3DTypeTopoBrepData,
                    [](A3DEntity *ntt) {
                        auto const ntt_type = getEntityType( ntt );
                        if( kA3DTypeRiBrepModel == ntt_type ) {
                            A3DRiBrepModelWrapper d( ntt );
                            return EntityArray( 1, d->m_pBrepData );
                        } else if( kA3DTypeRiPlane == ntt_type ) {
                            A3DRiPlaneWrapper d( ntt );
                            return EntityArray( 1, d->m_pBrepData );
                        }
                        return EntityArray();
                    }
                },
                {
                    kA3DTypeTopoSingleWireBody,
                    [](A3DEntity *ntt) {
                        if( kA3DTypeRiCurve == getEntityType( ntt ) ) {
                            A3DRiCurveWrapper d( ntt );
                            return EntityArray( 1, d->m_pBody );
                        }
                        return EntityArray();
                    }
                }
            }
        },
        {
            kA3DTypeTopoBrepData,
            {
                {
                    kA3DTypeTopoConnex,
                    [](A3DEntity *ntt) {
                        A3DTopoBrepDataWrapper d( ntt );
                        return toVector( d->m_ppConnexes, d->m_uiConnexSize );
                    }
                }
            }
        },
        {
            kA3DTypeTopoConnex,
            {
                {
                    kA3DTypeTopoShell,
                    [](A3DEntity *ntt) {
                        A3DTopoConnexWrapper d( ntt );
                        return toVector( d->m_ppShells, d->m_uiShellSize );
                    }
                }
            }
        },
        {
            kA3DTypeTopoShell,
            {
                {
                    kA3DTypeTopoFace,
                    [](A3DEntity *ntt) {
                        A3DTopoShellWrapper d( ntt );
                        return toVector( d->m_ppFaces, d->m_uiFaceSize );
                    }
                }
            }
        },
        {
            kA3DTypeTopoFace,
            {
                {
                    kA3DTypeTopoLoop,
                    [](A3DEntity *ntt) {
                        A3DTopoFaceWrapper d( ntt );
                        return toVector( d->m_ppLoops, d->m_uiLoopSize );
                    }
                }
            }
        },
        {
            kA3DTypeTopoLoop,
            {
                {
                    kA3DTypeTopoCoEdge,
                    [](A3DEntity *ntt) {
                        A3DTopoLoopWrapper d( ntt );
                        return toVector( d->m_ppCoEdges, d->m_uiCoEdgeSize );
                    }
                }
            }
        },
        {
            kA3DTypeTopoCoEdge,
            {
                {
                    kA3DTypeTopoEdge,
                    []( A3DEntity *ntt ) {
                        A3DTopoCoEdgeWrapper d( ntt );
                        return EntityArray( 1, d->m_pEdge );
                        
                    }
                }
            }
        },
        {
            kA3DTypeTopoEdge,
            {
                {
                    kA3DTypeTopoVertex,
                    []( A3DEntity *ntt ) {
                        A3DTopoEdgeWrapper d( ntt );
                        EntityArray result;
                        result.reserve( 2 );
                        result.push_back( d->m_pStartVertex );
                        result.push_back( d->m_pEndVertex );
                        return result;
                    }
                }
            }
        },
        {
            kA3DTypeTopoSingleWireBody,
            {
                {
                    kA3DTypeTopoWireEdge,
                    []( A3DEntity *ntt ) {
                        A3DTopoSingleWireBodyWrapper d( ntt );
                        return EntityArray( 1, d->m_pWireEdge );
                    }
                }
            }
        },
        {
            kA3DTypeMkpAnnotationEntity,
            {
                {
                    kA3DTypeMkpAnnotationEntity,
                    []( A3DEntity *ntt ) {
                        if( kA3DTypeMkpAnnotationSet == getEntityType( ntt ) ) {
                            A3DMkpAnnotationSetWrapper d( ntt );
                            return toVector( d->m_ppAnnotations, d->m_uiAnnotationsSize );
                        }
                        return EntityArray();
                    }
                },
                {
                    kA3DTypeMiscMarkupLinkedItem,
                    []( A3DEntity *ntt ) {
                        if( kA3DTypeMkpAnnotationReference == getEntityType( ntt ) ) {
                            A3DMkpAnnotationReferenceWrapper d( ntt );
                            return toVector( d->m_ppLinkedItems, d->m_uiLinkedItemsSize );
                        }
                        return EntityArray();
                    }
                },
                {
                    kA3DTypeMkpMarkup, // Base type, matches all markups
                    []( A3DEntity *ntt ) {
                        if( kA3DTypeMkpAnnotationItem == getEntityType( ntt ) ) {
                            A3DMkpAnnotationItemWrapper d( ntt );
                            return EntityArray( 1, d->m_pMarkup );
                        }
                        return EntityArray();
                    }
                }
            }
        },
        {
            kA3DTypeMkpMarkup,
            {
                {
                    kA3DTypeMiscMarkupLinkedItem,
                    []( A3DEntity *ntt ) {
                        A3DMkpMarkupWrapper d( ntt );
                        return toVector( d->m_ppLinkedItems, d->m_uiLinkedItemsSize );
                    }
                },
                {
                    kA3DTypeMkpLeader,
                    []( A3DEntity *ntt ) {
                        A3DMkpMarkupWrapper d( ntt );
                        return toVector( d->m_ppLeaders, d->m_uiLeadersSize );
                    }
                }
            }
        }
    };
    
    // For an entity type child_type, this function returns
    // a list of entity types that can contain the child type
    static inline TypeSet getPossibleParents( A3DEEntityType const &child_type ) {
        static std::unordered_map<A3DEEntityType, TypeSet> _cache;
        auto const it = _cache.find( child_type );
        if( std::end( _cache ) != it ) {
            return it->second;
        }
        
        TypeSet possible_parents;
        for( auto const owning_type_entry : _getterMapByType ) {
            auto const owning_type = owning_type_entry.first;
            for( auto const child_type_entry : owning_type_entry.second ) {
                if( child_type_entry.first == child_type ) {
                    possible_parents.insert( owning_type );
                    break;
                }
            }
        }
        _cache[child_type] = possible_parents;
        return possible_parents;
    }
    
    // Since some entity types "derived", this function is a helper
    // used to obtain a "base" type for entity_type
    static inline A3DEEntityType getBaseType( A3DEEntityType const &entity_type ) {
        auto base_type = entity_type;
        if( isRepresentationItem( entity_type ) ) {
            base_type = kA3DTypeRiRepresentationItem;
        } else if( isTessBase( entity_type ) ) {
            base_type = kA3DTypeTessBase;
        } else if( kA3DTypeTopoUniqueVertex == entity_type || kA3DTypeTopoMultipleVertex == entity_type ) {
            base_type = kA3DTypeTopoVertex;
        } else if( isAnnotationEntity( entity_type ) ) {
            base_type = kA3DTypeMkpAnnotationEntity;
        } else if( isMarkup( entity_type ) ) {
            base_type = kA3DTypeMkpMarkup;
        }
        return base_type;
    }
    
    // This function can be used to obtain a set of type paths where the parent type
    // and leaf child type values are both given. Each type path returned is one
    // possible way to traverse the Exchange data structures such that the beginning
    // entity type and ending entity type are equal to the input parameters.
    static TypePathArray getPossibleTypePaths( A3DEEntityType const &parent_type, A3DEEntityType const &child_type ) {
        
        // check to see if the given owner and child type values are present in the cache
        static std::unordered_map< A3DEEntityType, std::unordered_map< A3DEEntityType, TypePathArray >> _cache;
        auto const parent_type_it = _cache.find( parent_type );
        if(std::end( _cache ) != parent_type_it ) {
            auto const child_type_it = parent_type_it->second.find( child_type );
            if( std::end( parent_type_it->second ) != child_type_it ) {
                return child_type_it->second;
            }
        }
        
        // result is not present in the cache, so compute it
        TypePathArray type_paths;
        auto const possible_parents = getPossibleParents( child_type );
        if( std::end( possible_parents ) != possible_parents.find( parent_type ) ) {
            // child entity type is an immediate child of parent entity type
            TypePath type_path;
            type_path.push_back( parent_type );
            type_path.push_back( child_type );
            type_paths.push_back( type_path );
        } else {
            // child entity type is not an immediate child of parent entity type
            // so we must iterate over the list of all possible parent types
            // and for each of those parent types, examine up the ownership chain
            for( auto const intermediate_parent_type : possible_parents ) {
                if( intermediate_parent_type != child_type ) {
                    auto const intermediate_type_paths = getPossibleTypePaths( parent_type, intermediate_parent_type );
                    for( auto const intermediate_type_path : intermediate_type_paths ) {
                        type_paths.push_back( intermediate_type_path );
                        type_paths.back().push_back( child_type );
                    }
                }
            }
        }
        // augment the cache

        _cache[parent_type][child_type] = type_paths;
        return type_paths;
    }
    
    static ts3d::InstancePathArray getInstancePaths( A3DEntity *owner, TypePath const &type_path, ts3d::InstancePath const &instance_path = ts3d::InstancePath() )  {
        ts3d::InstancePathArray instance_paths;
        auto const owner_type = ts3d::getEntityType( owner );
        auto const owner_base_type = getBaseType( owner_type );
        auto const owner_decomposer_it = _getterMapByType.find( owner_base_type );
        auto base_instance_path = instance_path;
        base_instance_path.push_back( owner );

        if(std::end( _getterMapByType ) == owner_decomposer_it ) {
            if( type_path.size() == 1 && owner_base_type == type_path.back() ) {
                instance_paths.push_back( base_instance_path );
            }
            return instance_paths;
        }
        
        
        if( kA3DTypeAsmProductOccurrence == owner_base_type ) {
            auto const children = owner_decomposer_it->second[kA3DTypeAsmProductOccurrence]( owner );
            if( !children.empty() ) {
                for( auto child : children ) {
                    auto const child_instance_paths = getInstancePaths( child, type_path, base_instance_path );
                    std::copy( child_instance_paths.begin(), child_instance_paths.end(), std::back_inserter( instance_paths ) );
                }
            }
        } else if( kA3DTypeRiSet == owner_type ) {
            auto const children = owner_decomposer_it->second[kA3DTypeRiRepresentationItem]( owner );
            if( !children.empty() ) {
                for( auto child : children ) {
                    auto const child_instance_paths = getInstancePaths( child, type_path, base_instance_path );
                    std::copy( child_instance_paths.begin(), child_instance_paths.end(), std::back_inserter( instance_paths ) );
                }
                return instance_paths;
            }
        } else if( kA3DTypeMkpAnnotationSet == owner_type ) {
            auto const children = owner_decomposer_it->second[kA3DTypeMkpAnnotationEntity]( owner );
            if( !children.empty() ) {
                for( auto child : children ) {
                    auto const child_instance_paths = getInstancePaths( child, type_path, base_instance_path );
                    std::copy( child_instance_paths.begin(), child_instance_paths.end(), std::back_inserter( instance_paths ) );
                }
            }
        }
        
        TypePath const child_type_path( type_path.begin() + 1, type_path.end() );
        if(child_type_path.empty()) {
            instance_paths.push_back( base_instance_path );
            return instance_paths;
        }
        auto const children = owner_decomposer_it->second[child_type_path.front()]( owner );
        for(auto child : children ) {
            auto const child_instance_paths = getInstancePaths( child, child_type_path, base_instance_path );
            std::copy( child_instance_paths.begin(), child_instance_paths.end(), std::back_inserter( instance_paths ) );
        }
        return instance_paths;
    }
}

ts3d::InstancePathArray ts3d::getLeafInstances( A3DEntity *owner, A3DEEntityType const &leaf_type ) {
    InstancePathArray result;
    auto const owner_base_type = getBaseType( getEntityType( owner ) );
    auto const leaf_base_type = getBaseType( leaf_type );
    
    auto const possible_type_paths = getPossibleTypePaths( owner_base_type, leaf_base_type );
    for( auto const possible_type_path : possible_type_paths ) {
        if( possible_type_path.empty() ) {
            continue;
        }
        auto const instance_paths = getInstancePaths( owner, possible_type_path );
        if( leaf_base_type != leaf_type ) {
            std::copy_if( instance_paths.begin(), instance_paths.end(), std::back_inserter( result ),
                         [leaf_type](InstancePath const &instance_path) {
                             auto const leaf_entity = instance_path.back();
                             return getEntityType( leaf_entity ) == leaf_type;
                         });
        } else {
            std::copy( instance_paths.begin(), instance_paths.end(), std::back_inserter( result ) );
        }
    }
    
    return result;
}

ts3d::EntitySet ts3d::getUniqueParts( A3DAsmModelFile *modelFile ) {
    return getUniqueLeafEntities( modelFile, kA3DTypeAsmPartDefinition );
}

ts3d::EntitySet ts3d::getUniqueLeafEntities( A3DEntity *owner, A3DEEntityType const &leaf_type ) {
    EntitySet result;
    auto const paths = getLeafInstances( owner, leaf_type );
    for( auto path : paths ) {
        result.insert( path.back() );
    }
    return result;
}

ts3d::EntitySet ts3d::getUniqueLeafEntities( A3DEntity *owner, A3DEEntityType const &leaf_type, InstancePathMap &instance_path_map ) {
    EntitySet result;
    auto const paths = getLeafInstances( owner, leaf_type );
    for( auto path : paths) {
        result.insert( path.back() );
        instance_path_map[path.back()].emplace_back( path );
    }
    return result;
}

ts3d::InstancePath ts3d::getOwningInstance( InstancePath const &instance_path, A3DEEntityType const &owner_type ) {
    auto result = instance_path;
    while( !result.empty() && getEntityType( result.back() ) != owner_type ) {
        result.pop_back();
    }
    return result;
}

ts3d::EntityArray ts3d::getChildren( A3DEntity *owner, A3DEEntityType const &child_type ) {
    auto const owner_type = ts3d::getEntityType( owner );
    auto const owner_base_type = getBaseType( owner_type );
    auto const child_base_type = getBaseType( child_type );
    auto const owner_decomposer_it = _getterMapByType.find( owner_base_type );
    
    EntityArray result;
	auto const decomposer_it = owner_decomposer_it->second.find(child_base_type);
	if (std::end(owner_decomposer_it->second) == decomposer_it) {
		return result;
	}
    auto const children = decomposer_it->second( owner );
    std::copy_if( std::begin( children ), std::end( children ), std::back_inserter( result ), [child_type, child_base_type]( A3DEntity *child ) {
        return child_type == child_base_type || getEntityType( child ) == child_type;
    });
    return result;
}


namespace {
    static inline std::string getName( A3DEntity *ntt ) {
        ts3d::A3DRootBaseWrapper d( ntt );
        if( d->m_pcName ) {
            return d->m_pcName;
        }

        auto const t = ts3d::getEntityType( ntt );
        if( kA3DTypeAsmProductOccurrence == t ) {
            ts3d::A3DAsmProductOccurrenceWrapper d2( ntt );
            return getName( d2->m_pPrototype );
        }
        
        return std::string();
    }

    A3DMiscTransformation *getLocation( A3DAsmProductOccurrence *po ) {
        if( nullptr == po ) {
            return nullptr;
        }
        A3DAsmProductOccurrenceData d;
        A3D_INITIALIZE_DATA( A3DAsmModelFileData, d );
        A3DAsmProductOccurrenceGet( po, &d );
        auto location = d.m_pLocation;
        auto prototype = d.m_pPrototype;
        A3DAsmProductOccurrenceGet( nullptr, &d );
        return location ? location : getLocation( prototype );
    }
}

namespace ts3d {
    class Instance;
    /*! \brief Abstraction of a shared pointer to an Instance object.
     *  \ingroup access
     */
    using InstancePtr = std::shared_ptr<Instance>;
    
    /*! \brief An instance should be thought of as a specific path through
     * the Exchange product structure to a particular leaf entity.
     *
     * Since the path to an entity provides context into how attributes
     * should be interpretted, this class encapsulates the information
     * via its InstancePath.
     * \ingroup access
     */
    class Instance {
    public:
        /*! \brief Constructs an Instance from the provided InstancePath
         */
        Instance( InstancePath const &path )
        : _path( path ) {
        }

        
        /*! \brief Copy constructor
         */
        Instance( Instance const &other )
        : _path( other._path ) {
        }

        
        /*! \brief Move constructor
         */
        Instance( Instance &&other )
        : _path( std::move( other._path ) ) {
        }

        
        /*! \brief Destructor
         */
        virtual ~Instance( void ) {
            reset( InstancePath() );
        }
        
        /*! \brief Assignment
         */
        Instance &operator=( Instance const &other ) {
            _path = other._path;
            return *this;
        }
        
        /*! \brief Move assign
         */
        Instance &operator=( Instance &&other ) {
            _path = std::move( other._path );
            return *this;
        }

        /*! \brief Compare paths
         */
        bool operator==( Instance const &other ) const {
            return _path == other._path;
        }
        
        /*! \brief Gets the object name
         */
        std::string getName( void ) const {
            return ::getName( leaf() );
        }

        /*! \brief Gets the object type as a string
         */
        std::string getType( void ) const {
            return A3DMiscGetEntityTypeMsg( leafType() );
        }

        /*! \brief Gets the InstancePath this Instance references
         */
        InstancePath const &path( void ) const {
            return _path;
        }

        /*! \brief Gets an Instance object for the parent
         */
        Instance owner( void ) const {
            if( _path.size() < 2 ) {
                return Instance( InstancePath() );
            }
            auto owner_path = _path;
            owner_path.pop_back();
            return Instance( owner_path );
        }


        /*! \brief Resets the InstancePath this Instance references
         */
        void reset( InstancePath const &new_path ) {
            if( new_path == _path ) {
                return;
            }
            
            _path = new_path;
            
            for( auto attrib : _cascaded_attribs ) {
                A3DMiscCascadedAttributesDelete( attrib );
            }
            
            _cascaded_attribs_data.reset( nullptr );
        }
        
        /*! \brief Gets the leaf entity pointer
         */
        A3DEntity *leaf( void ) const {
            return _path.empty() ? nullptr : _path.back();
        }

        /*! \brief Gets the type of the leaf entity
         */
        A3DEEntityType leafType( void ) const {
           return getEntityType( leaf() );
       }



		/** @name Net attribute getters */
		/**@{*/
        /*! \brief Gets the net removed
         */
        bool getNetRemoved( void ) const {
            return A3D_TRUE == getCascadedAttributesData()->m_bRemoved;
        }


        /*! \brief Gets the net show
         */
        bool getNetShow( void ) const {
            return A3D_TRUE == getCascadedAttributesData()->m_bShow;
        }
        
        /*! \brief Gets the net style
         */
        A3DGraphStyleData getNetStyle( void ) const {
            return getCascadedAttributesData()->m_sStyle;
        }
        
        /*! \brief Gets the net layer
         */
        A3DUns16 getNetLayer( void ) const {
            return getCascadedAttributesData()->m_usLayer;
        }
        /**@}*/

	protected:
        /*! \private */
        A3DMiscCascadedAttributesWrapper const &getCascadedAttributesData( void ) const {
            if( _cascaded_attribs.empty() ) {
                getCascadedAttributes();
            }
            return _cascaded_attribs_data;
        }
        
        /*! \private */
        A3DMiscCascadedAttributes *getCascadedAttributes( void ) const {
            if(_cascaded_attribs.empty() ) {
                _cascaded_attribs.push_back( nullptr );
                CheckResult( A3DMiscCascadedAttributesCreate( &_cascaded_attribs.back() ) );
                for( auto ntt : _path ) {
                    if( A3DEntityIsBaseWithGraphicsType( ntt ) ) {
                        auto father = _cascaded_attribs.back();
                        _cascaded_attribs.push_back( nullptr );
                        CheckResult( A3DMiscCascadedAttributesCreate( &_cascaded_attribs.back() ) );
                        CheckResult( A3DMiscCascadedAttributesPush( _cascaded_attribs.back(), ntt, father ) );
                    }
                }
                if( !_cascaded_attribs.empty() ) {
                    _cascaded_attribs_data.reset( _cascaded_attribs.back() );
                }
            }
            return _cascaded_attribs.empty() ? nullptr : _cascaded_attribs.back();
        }
        
        /*! \private */
        InstancePath _path;
        /*! \private */
        mutable std::vector<A3DMiscCascadedAttributes*> _cascaded_attribs;
        /*! \private */
        mutable A3DMiscCascadedAttributesWrapper _cascaded_attribs_data;
    };

	/*! \brief Base class for easing access to tessellation data.
	 * Provides access to coordinate values.
	 * \ingroup access
	 */
    class TessBaseInstance : public Instance {
    public:
        /*! \brief throws if leaf type is not "derived" from A3DTessBase
         */
        TessBaseInstance( InstancePath const &path )
        : Instance( path ) {
            if( !isTessBase( leafType() ) ) {
                throw std::invalid_argument( "Expected leaf node type of A3DTessBase. " );
            }
            _d.reset( leaf() );
        }

        
        /*! \brief Gets the array of coordinate values for the tesselation.
         * This array is of length coordsSize().
         */
        double const *coords( void ) const {
            return _d->m_pdCoords;
        }

        /*! \brief The length of the coords() array.
         */
        A3DUns32 coordsSize( void ) const {
            return _d->m_uiCoordSize;
        }
        
    private:
        A3DTessBaseWrapper _d;
    };

    /*! \brief This is a helper class used to more easily provide access to the
     * indexed mesh data for a given face.
     * \ingroup access
     */
    class TessFaceDataHelper {
    public:
        /*! \private */
        TessFaceDataHelper( A3DTessFaceData const &d, A3DUns32 const *triangulatedIndexes, A3DUns32 const *wireIndexes ) {
            auto sz_tri_idx = 0u;
            auto ti_index = d.m_uiStartTriangulated;
            if(kA3DTessFaceDataTriangle & d.m_usUsedEntitiesFlags) {
                auto const num_tris = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto tri = 0u; tri < num_tris; tri++) {
                    for(auto vert = 0u; vert < 3u; vert++) {
                        _normals.push_back( triangulatedIndexes[ti_index++] );
                        _vertices.push_back( triangulatedIndexes[ti_index++] );
                    }
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleFan  & d.m_usUsedEntitiesFlags) {
                auto const num_fans = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto fan_idx = 0u; fan_idx < num_fans; ++fan_idx) {
                    auto const num_pts = d.m_puiSizesTriangulated[sz_tri_idx++];
                    auto const root_n = triangulatedIndexes[ti_index++];
                    auto const root_v = triangulatedIndexes[ti_index++];
                    for(auto vert = 1u; vert < num_pts - 1u; vert++) {
                        _normals.push_back( root_n );
                        _vertices.push_back( root_v );
                        
                        _normals.push_back( triangulatedIndexes[ti_index++] );
                        _vertices.push_back( triangulatedIndexes[ti_index++] );
                        
                        _normals.push_back( triangulatedIndexes[ti_index] );
                        _vertices.push_back( triangulatedIndexes[ti_index + 1] );
                    }
                    ti_index += 2;
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleStripe & d.m_usUsedEntitiesFlags) {
                auto const num_tri_stips = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto strip_idx = 0u; strip_idx < num_tri_stips; ++strip_idx) {
                    auto const num_pts = d.m_puiSizesTriangulated[sz_tri_idx++];
                    ti_index += 2;
                    for(auto vert = 1u; vert < num_pts - 1u; vert++) {
                        auto const prev_n = triangulatedIndexes[ti_index - 2];
                        auto const prev_v = triangulatedIndexes[ti_index - 1];
                        auto const current_n = triangulatedIndexes[ti_index++];
                        auto const current_v = triangulatedIndexes[ti_index++];
                        auto const next_n = triangulatedIndexes[ti_index];
                        auto const next_v = triangulatedIndexes[ti_index + 1];
                        
                        _normals.push_back( current_n );
                        _vertices.push_back( current_v );
                        _normals.push_back( vert % 2 ? next_n : prev_n );
                        _vertices.push_back( vert % 2 ? next_v : prev_v );
                        _normals.push_back( vert % 2 ? prev_n : next_n );
                        _vertices.push_back( vert % 2 ? prev_v : next_v );
                    }
                    ti_index += 2;
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleOneNormal & d.m_usUsedEntitiesFlags) {
                auto const num_tris_1normal = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto tri = 0u; tri < num_tris_1normal; tri++) {
                    auto n = triangulatedIndexes[ti_index++];
                    for(auto vert = 0u; vert < 3u; vert++) {
                        _normals.push_back( n );
                        _vertices.push_back( triangulatedIndexes[ti_index++] );
                    }
                }
            }
            
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleFanOneNormal & d.m_usUsedEntitiesFlags) {
                auto const num_fans_1normal = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto fan_idx = 0u; fan_idx < num_fans_1normal; ++fan_idx) {
                    auto const has_vertex_normals = 0 == (d.m_puiSizesTriangulated[sz_tri_idx] & kA3DTessFaceDataNormalSingle);
                    auto const num_pts = d.m_puiSizesTriangulated[sz_tri_idx++] & kA3DTessFaceDataNormalMask;
                    auto const root_n = triangulatedIndexes[ti_index++];
                    auto const root_v = triangulatedIndexes[ti_index++];
                    for(auto vert = 1u; vert < num_pts - 1u; vert++) {
                        auto const n = (has_vertex_normals ? triangulatedIndexes[ti_index++] : root_n);
                        auto const v = triangulatedIndexes[ti_index++];
                        
                        auto const next_n = (has_vertex_normals ? triangulatedIndexes[ti_index] : root_n);
                        auto const next_v = triangulatedIndexes[ti_index + (has_vertex_normals ? 1 : 0)];
                        
                        _normals.push_back( root_n );
                        _vertices.push_back( root_v );
                        _normals.push_back( n );
                        _vertices.push_back( v );
                        _normals.push_back( next_n );
                        _vertices.push_back( next_v );
                    }
                    ti_index += (has_vertex_normals ? 2 : 1);
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleStripeOneNormal & d.m_usUsedEntitiesFlags) {
                auto const num_tri_stips = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto strip_idx = 0u; strip_idx < num_tri_stips; ++strip_idx) {
                    auto const has_vertex_normals = 0 == (d.m_puiSizesTriangulated[sz_tri_idx] & kA3DTessFaceDataNormalSingle);
                    auto const series_normal = triangulatedIndexes[ti_index];
                    auto const num_pts = d.m_puiSizesTriangulated[sz_tri_idx++] & kA3DTessFaceDataNormalMask;
                    ti_index += 2;
                    for(auto vert = 1u; vert < num_pts - 1u; vert++) {
                        auto const prev_n = (has_vertex_normals ? triangulatedIndexes[ti_index - 2] : series_normal);
                        auto const prev_v = triangulatedIndexes[ti_index - 1];
                        
                        auto const current_n = (has_vertex_normals ? triangulatedIndexes[ti_index++] : series_normal);
                        auto const current_v = triangulatedIndexes[ti_index++];
                        
                        auto const next_n = (has_vertex_normals ? triangulatedIndexes[ti_index] : series_normal);
                        auto const next_v = triangulatedIndexes[ti_index + (has_vertex_normals ? 1 : 0)];
                        
                        _normals.push_back( current_n );
                        _vertices.push_back( current_v );
                        _normals.push_back( vert % 2 ? next_n : prev_n );
                        _vertices.push_back( vert % 2 ? next_v : prev_v );
                        _normals.push_back( vert % 2 ? prev_n : next_n );
                        _vertices.push_back( vert % 2 ? prev_v : next_v );
                    }
                    ti_index += (has_vertex_normals ? 2 : 1);
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleTextured & d.m_usUsedEntitiesFlags) {
                auto const num_tris = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto tri_idx = 0u; tri_idx < num_tris; tri_idx++) {
                    for(auto vert_idx = 0u; vert_idx < 3u; vert_idx++) {
                        _normals.push_back( triangulatedIndexes[ti_index++] );
                        for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                            _texture.push_back( triangulatedIndexes[ti_index++] );
                        }
                        _vertices.push_back( triangulatedIndexes[ti_index++] );
                    }
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleFanTextured & d.m_usUsedEntitiesFlags) {
                auto const num_fans_textured = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto fan_idx = 0u; fan_idx < num_fans_textured; ++fan_idx) {
                    auto const num_pts = d.m_puiSizesTriangulated[sz_tri_idx++];
                    auto const n = triangulatedIndexes[ti_index++];
                    std::vector<A3DUns32> t;
                    for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                        t.push_back( triangulatedIndexes[ti_index++] );
                    }
                    auto const v = triangulatedIndexes[ti_index++];
                    for(auto vert = 1u; vert < num_pts - 1u; vert++) {
                        _normals.push_back( n );
                        _texture.insert( _texture.end(), t.begin(), t.end() );
                        _vertices.push_back( v );
                        
                        _normals.push_back( triangulatedIndexes[ti_index++] );
                        for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                            _texture.push_back( triangulatedIndexes[ti_index++] );
                        }
                        _vertices.push_back( triangulatedIndexes[ti_index++] );
                        
                        _normals.push_back( triangulatedIndexes[ti_index++] );
                        for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                            _texture.push_back( triangulatedIndexes[ti_index++] );
                        }
                        _vertices.push_back( triangulatedIndexes[ti_index++] );
                    }
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleStripeTextured  & d.m_usUsedEntitiesFlags) {
                auto const num_tri_stips = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto strip_idx = 0u; strip_idx < num_tri_stips; ++strip_idx) {
                    auto const num_pts = d.m_puiSizesTriangulated[sz_tri_idx++];
                    for(auto vert = 1u; vert < num_pts - 1u; vert++) {
                        auto const prev_n = triangulatedIndexes[ti_index - (2 + d.m_uiTextureCoordIndexesSize)];
                        std::vector<A3DUns32> prev_t;
                        for( auto i = ti_index - (1 + d.m_uiTextureCoordIndexesSize); i < ti_index - 1; ++i ) {
                            prev_t.push_back( triangulatedIndexes[i] );
                        }
                        auto const prev_v = triangulatedIndexes[ti_index - 1];
                        
                        auto const current_n = triangulatedIndexes[ti_index++];
                        std::vector<A3DUns32> current_t;
                        for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                            current_t.push_back( triangulatedIndexes[ti_index++] );
                        }
                        auto const current_v = triangulatedIndexes[ti_index++];
                        
                        auto const next_n = triangulatedIndexes[ti_index];
                        std::vector<A3DUns32> next_t;
                        for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                            next_t.push_back( triangulatedIndexes[ ti_index + i + 1] );
                        }
                        auto const next_v = triangulatedIndexes[ti_index + 1 + d.m_uiTextureCoordIndexesSize];
                        
                        _normals.push_back( current_n );
                        _texture.insert( _texture.end(), current_t.begin(), current_t.end() );
                        _vertices.push_back( current_v );
                        
                        _normals.push_back( vert % 2 ? next_n : prev_n );
                        _texture.insert( _texture.end(), vert % 2 ? next_t.begin() : prev_t.begin(), vert % 2 ? next_t.end() : prev_t.end() );
                        _vertices.push_back( vert % 2 ? next_v : prev_v );
                        
                        _normals.push_back( vert % 2 ? prev_n : next_n );
                        _texture.insert( _texture.end(), vert % 2 ? prev_t.begin() : next_t.begin(), vert % 2 ? prev_t.end() : next_t.end() );
                        _vertices.push_back( vert % 2 ? prev_v : next_v );
                    }
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleOneNormalTextured  & d.m_usUsedEntitiesFlags) {
                auto const has_vertex_normals = 0 == (d.m_puiSizesTriangulated[sz_tri_idx] & kA3DTessFaceDataNormalSingle);
                auto const series_normal = triangulatedIndexes[ti_index];
                auto const num_tris_1normal_textured = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto tri = 0u; tri < num_tris_1normal_textured; tri++) {
                    for(auto vert = 0u; vert < 3u; vert++) {
                        _normals.push_back( has_vertex_normals ? triangulatedIndexes[ti_index++] : series_normal );
                        for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                            _texture.push_back( triangulatedIndexes[ti_index++] );
                        }
                        _vertices.push_back( triangulatedIndexes[ti_index++] );
                    }
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleFanOneNormalTextured & d.m_usUsedEntitiesFlags) {
                auto const num_fans_textured = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto fan_idx = 0u; fan_idx < num_fans_textured; ++fan_idx) {
                    auto const root_n = triangulatedIndexes[ti_index++];
                    std::vector<A3DUns32> root_t;
                    for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                        root_t.push_back( triangulatedIndexes[ti_index++] );
                    }
                    auto const root_v = triangulatedIndexes[ti_index++];
                    auto const has_vertex_normals = 0 == (d.m_puiSizesTriangulated[sz_tri_idx] & kA3DTessFaceDataNormalSingle);
                    auto const num_pts = d.m_puiSizesTriangulated[sz_tri_idx++] & kA3DTessFaceDataNormalMask;
                    for(auto vert = 1u; vert < num_pts - 1u; vert++) {
                        _normals.push_back( root_n );
                        _texture.insert( _texture.end(), root_t.begin(), root_t.end() );
                        _vertices.push_back( root_v );
                        
                        _normals.push_back( has_vertex_normals ? triangulatedIndexes[ti_index++] : root_n );
                        for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                            _texture.push_back( triangulatedIndexes[ti_index++] );
                        }
                        _vertices.push_back( triangulatedIndexes[ti_index++] );
                        
                        _normals.push_back( has_vertex_normals ? triangulatedIndexes[ti_index] : root_n );
                        for( auto i = 0u; i < d.m_uiTextureCoordIndexesSize; ++i ) {
                            _texture.push_back( triangulatedIndexes[ti_index + i + 1] );
                        }
                        _vertices.push_back( triangulatedIndexes[ti_index + 1 + (has_vertex_normals ? d.m_uiTextureCoordIndexesSize : 0)] );
                    }
                }
            }
            if(d.m_uiSizesTriangulatedSize > sz_tri_idx && kA3DTessFaceDataTriangleStripeOneNormalTextured & d.m_usUsedEntitiesFlags) {
                auto const num_tri_stips = d.m_puiSizesTriangulated[sz_tri_idx++];
                for(auto strip_idx = 0u; strip_idx < num_tri_stips; ++strip_idx) {
                    auto const has_vertex_normals = 0 == (d.m_puiSizesTriangulated[sz_tri_idx] & kA3DTessFaceDataNormalSingle);
                    auto const num_pts = d.m_puiSizesTriangulated[sz_tri_idx++] & kA3DTessFaceDataNormalMask;
                    auto const series_normal = triangulatedIndexes[ti_index];
                    ti_index += 2 + d.m_uiStyleIndexesSize;
                    for(auto vert = 1u; vert < num_pts - 1u; vert++) {
                        auto const prev_n = has_vertex_normals ? triangulatedIndexes[ti_index - (2 + d.m_uiTextureCoordIndexesSize)] : series_normal;
                        auto const prev_v = triangulatedIndexes[ti_index - 1];
                        
                        
                        auto const current_n = has_vertex_normals ? triangulatedIndexes[ti_index++] : series_normal;
                        ti_index += d.m_uiTextureCoordIndexesSize;
                        auto const current_v = triangulatedIndexes[ti_index++];
                        
                        auto const next_n = has_vertex_normals ? triangulatedIndexes[ti_index] : series_normal;
                        auto const next_v = triangulatedIndexes[ti_index + 1 + (has_vertex_normals ? d.m_uiTextureCoordIndexesSize : 0)];
                        
                        _normals.push_back( current_n );
                        _vertices.push_back( current_v );
                        _normals.push_back( vert % 2 ? next_n : prev_n );
                        _vertices.push_back( vert % 2 ? next_v : prev_v );
                        _normals.push_back( vert % 2 ? prev_n : next_n );
                        _vertices.push_back( vert % 2 ? prev_v : next_v );
                    }
                }
            }
            
            auto wi_index = d.m_uiStartWire;
            TessLoop current_loop;
            for( auto idx = 0u; idx < d.m_uiSizesWiresSize; ++idx ) {
                auto const nverts_with_flags = d.m_puiSizesWires[idx];
                auto const nvertices = nverts_with_flags & ~kA3DTessFaceDataWireIsClosing & ~kA3DTessFaceDataWireIsNotDrawn;
                auto const is_closing = nverts_with_flags & kA3DTessFaceDataWireIsClosing;
                auto const is_hidden = nverts_with_flags & kA3DTessFaceDataWireIsNotDrawn;
                TessEdge current_edge;
                current_edge._visible = !is_hidden;
                for(auto vidx = 0u; vidx < nvertices; ++vidx ) {
                    current_edge._vertices.push_back( wireIndexes[wi_index++] );
                }
                current_loop._edges.push_back( current_edge );
                
                if(is_closing) {
                    _loops.push_back( current_loop );
                    current_loop._edges.clear();
                }
            }
        }
        /*! \private */
        TessFaceDataHelper( TessFaceDataHelper const &other )
        : _vertices( other._vertices ), _normals( other._normals ), _texture( other._texture ) {
        }

        /*! \private */
        TessFaceDataHelper( TessFaceDataHelper &&other )
        : _vertices( std::move( other._vertices ) ), _normals( std::move( other._normals ) ), _texture( std::move( other._texture ) ) {
        }

        
        /*! \private */
        TessFaceDataHelper &operator=( TessFaceDataHelper const &other ) {
            _vertices = other._vertices;
            _normals = other._normals;
            _texture = other._texture;
            return *this;
        }
        /*! \private */
        TessFaceDataHelper &operator=( TessFaceDataHelper &&other ) {
            _vertices = std::move( other._vertices );
            _normals = std::move( other._normals );
            _texture = std::move( other._texture );
            return *this;
        }
        
        /*! \brief Gets the list of vertex index values defining the 
         *  triangles of the mesh for this face. The length of
         *  this vector should be a multiple of 3. Index values should
         *  be accessed as triplets, each value defining the offset in
         *  the TessBaseInstance::coords() array.
         */
        std::vector<A3DUns32> const &vertices( void ) const {
            return _vertices;
        }

        /*! \brief Gets the list of vertex index values defining the
         *  normal vectors at each vertex location. The length of
         *  this vector should be a multiple of 3. Index values should
         *  be accessed as triplets, each value defining the offset in
         *  the Tess3DInstance::normals() array.
         */
        std::vector<A3DUns32> const &normals( void ) const {
            return _normals;
        }

        /*! \brief Gets the list of texture coordinate values.
         *  \todo Explain this better
         */
        std::vector<A3DUns32> const &textures( void ) const {
            return _texture;
        }
        
        /*! \brief Index list of vertices for the tessellation of
         *  a specific edge.
         */
        struct TessEdge {
            /*! \brief Array indexes for the vertices */
            std::vector<A3DUns32> _vertices;
            /*! \brief Visibility flag */
            bool _visible;
        };

        /*! \brief A collection of TessEdge objects representing
         *  a single closed series of edges that bound a face.
         */
        struct TessLoop {
            /*! \brief Collection of edges that make up this loop */
            std::vector<TessEdge> _edges;
        };
        
        /*! \brief Provides a list of TessLoop objects representing the
         * edge loops that bound this face.
         */
        std::vector<TessLoop> const loops( void ) const {
            return _loops;
        }
        
    private:
        std::vector<A3DUns32> _vertices, _normals, _texture;
        std::vector<TessLoop> _loops;
    };

    /*! \brief Encapsulates the functionality desired to easily retrieve
     * normal and texture coordinates for a tessellation.
     * \ingroup access
     */
    class Tess3DInstance : public TessBaseInstance {
    public:
        /*! \brief Throws if leaf type is not kA3DTypeTess3D
         */
        Tess3DInstance( InstancePath const &path )
        : TessBaseInstance( path ) {
            if( kA3DTypeTess3D != leafType() ) {
                throw std::invalid_argument("Expected A3DTess3D leaf not type." );
            }
            _d.reset( leaf() );
        }

        /*! \brief Obtains a pointer to an array of normal coordinates.
         *  The length of this array is determined by normalsSize()
         */
        double const *normals( void ) const {
            return _d->m_pdNormals;
        }

        /*! \brief Provides the size of the normal array.
         */
        A3DUns32 normalsSize( void ) const {
            return _d->m_uiNormalSize;
        }
        
        /*! \brief Obtains a pointer to an array of texture coordinates.
         *  The length of this array is determined by texCoordsSize()
         */
        double const *texCoords( void ) const {
            return _d->m_pdTextureCoords;
        }

        /*! \brief Provides the length of the texCoords array.
         */
        A3DUns32 texCoordsSize( void ) const {
            return _d->m_uiTextureCoordSize;
        }
        
        /*! \brief Provides the total number of faces with available
         *  tessellation data. \sa getIndexMeshForFace
         */
        A3DUns32 faceSize( void ) const {
            return _d->m_uiFaceTessSize;
        }

        /*! \brief Gets an object that wraps the tessellation data of
         *  a given face.
         * \param face_idx Index of the face to obtain the access helper for.
         *  Valid index values must fall in the interval [0, faceSize()).
         */
        TessFaceDataHelper getIndexMeshForFace( A3DUns32 const &face_idx ) const {
            if( face_idx >= _d->m_uiFaceTessSize ) {
                throw std::out_of_range( "Index of face requested for index mesh is out of range." );
            }
            return TessFaceDataHelper( _d->m_psFaceTessData[face_idx], _d->m_puiTriangulatedIndexes, _d->m_puiWireIndexes );
        }
        
    private:
        A3DTess3DWrapper _d;
    };

    /*! \brief Provides easy access to wire tessellation data.
     *  \ingroup access
     *  \todo Incomplete
     */
    class Tess3DWireInstance : public TessBaseInstance {
    public:
        /*! \brief Constructor
         */
        Tess3DWireInstance( InstancePath const &path );
        
        /*! \brief Gets the number of wires
         */
        A3DUns32 getNumberOfWires( void ) const;
        
        /*! \brief Structure describing wire data
         */
        struct WireData {
            /*! \brief The vertex index values */
            std::vector<A3DUns32> _vertices;
            /*! \brief Indicated if the wire is closing */
            bool _closed;
            /*! \brief Indicates if the wire is connected to the previous wire */
            bool _connected_to_previous;
        };
        
        /*! \brief How color is specified
         */
        enum class ColorSpec {
            /*! \brief RGB only, no alpha */
            RGB,
            /*! \brief RGB and alpha.. imagine that! */
            RGBA
        };

        /*! \brief Hos is the color specified? */
        ColorSpec getColorSpecification( void ) const;
        
        /*! \brief How is the color applied? */
        enum class ColorApplication {
            /*! \brief Per vertex color */
            PerVertex,
            /*! \brief Per wire segment color */
            PerSegment
        };
        
    private:
        /*! \private */
        A3DTess3DWireWrapper _d;
    };
}

namespace ts3d {
    /*! \brief This specific type of an Instance expects the leaf node
     * to be of type kA3DTypeRiRepresentationItem.
     * \ingroup access
     */
    class RepresentationItemInstance : public Instance {
    public:
        /*! \brief throws if leaf entity is not type kA3DTypeRiRepresentationItem
         */
        RepresentationItemInstance( InstancePath const &path )
        : Instance( path ) {
            if( !isRepresentationItem( leafType() ) ) {
                throw std::invalid_argument( "The leaf node in the instance path must be a representation item." );
            }
        }

        /*! \brief Provides a concrete type for the tessellation instance
         * Will be one of type:
         * - Tess3DInstance
         */
        std::shared_ptr<TessBaseInstance> getTessellation( void ) const {
            A3DRiRepresentationItemWrapper d( leaf() );
            auto tess_type = kA3DTypeUnknown;
            A3DEntityGetType( d->m_pTessBase, &tess_type );
            auto tess_path = path();
            tess_path.push_back( d->m_pTessBase );
            switch( tess_type ) {
                case kA3DTypeTess3D:
                    return std::make_shared<Tess3DInstance>( tess_path );
                    break;
                case kA3DTypeTess3DWire:
                    break;
                default:
                    break;
            }
            return nullptr;
        }
        
        /** @name Net attribute getters */
        /**@{*/
        /*! \brief Gets the net removed
         */
        bool getNetRemoved( A3DUns32 const face_idx ) const {
            return A3D_TRUE == getCascadedAttributesData( face_idx )->m_bRemoved;
        }


        /*! \brief Gets the net show
         */
        bool getNetShow( A3DUns32 const face_idx ) const {
            return A3D_TRUE == getCascadedAttributesData( face_idx )->m_bShow;
        }
        
        /*! \brief Gets the net style
         */
        A3DGraphStyleData getNetStyle( A3DUns32 const face_idx ) const {
            return getCascadedAttributesData( face_idx )->m_sStyle;
        }
        
        /*! \brief Gets the net layer
         */
        A3DUns16 getNetLayer( A3DUns32 const face_idx ) const {
            return getCascadedAttributesData( face_idx )->m_usLayer;
        }
        /**@}*/

    protected:
        A3DMiscCascadedAttributesWrapper getCascadedAttributesData( A3DUns32 const face_idx ) const {
            if( _cascaded_attribs.empty() ) {
                getCascadedAttributes();
            }

            auto cascaded_attribs_copy = _cascaded_attribs;
            auto father = cascaded_attribs_copy.back();
            cascaded_attribs_copy.push_back( nullptr );
            CheckResult( A3DMiscCascadedAttributesCreate( &cascaded_attribs_copy.back() ) );

            A3DRiRepresentationItemWrapper d( leaf() );
            A3DTess3DWrapper tess_d( d->m_pTessBase );
            CheckResult( A3DMiscCascadedAttributesPushTessFace( cascaded_attribs_copy.back(), leaf(), d->m_pTessBase, &tess_d->m_psFaceTessData[face_idx], face_idx, father ) );
            auto result = A3DMiscCascadedAttributesWrapper( cascaded_attribs_copy.back() );
            CheckResult( A3DMiscCascadedAttributesDelete( cascaded_attribs_copy.back() ) );
            return result;
        }
    };
    
    inline A3DVector3dData zeroVector( void ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = result.m_dY = result.m_dZ = 0.;
        return result;

    }
    
    inline A3DVector3dData operator+(A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = lhs.m_dX + rhs.m_dX;
        result.m_dY = lhs.m_dY + rhs.m_dY;
        result.m_dZ = lhs.m_dZ + rhs.m_dZ;
        return result;
    }
    
    inline A3DVector3dData &operator+=(A3DVector3dData &lhs, A3DVector3dData const &rhs ) {
        lhs = lhs + rhs;
        return lhs;
    }
    
    inline A3DVector3dData operator-(A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = lhs.m_dX - rhs.m_dX;
        result.m_dY = lhs.m_dY - rhs.m_dY;
        result.m_dZ = lhs.m_dZ - rhs.m_dZ;
        return result;
    }
    
    inline A3DVector3dData operator-( A3DVector3dData const &lhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = -lhs.m_dX;
        result.m_dY = -lhs.m_dY;
        result.m_dZ = -lhs.m_dZ;
        return result;
    }

    inline A3DVector3dData &operator-=(A3DVector3dData &lhs, A3DVector3dData const &rhs ) {
        lhs = lhs - rhs;
        return lhs;
    }

    inline A3DVector3dData operator*(A3DVector3dData const &lhs, double const &rhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = lhs.m_dX * rhs;
        result.m_dY = lhs.m_dY * rhs;
        result.m_dZ = lhs.m_dZ * rhs;
        return result;
    }

    inline A3DVector3dData operator*(double const &lhs, A3DVector3dData const &rhs ) {
        return rhs * lhs;
    }

    inline A3DVector3dData &operator*=(A3DVector3dData &lhs, double const &rhs ) {
        lhs.m_dX *= rhs;
        lhs.m_dY *= rhs;
        lhs.m_dZ *= rhs;
        return lhs;
    }
    
    inline A3DVector3dData operator/(A3DVector3dData const &lhs, double const &rhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = lhs.m_dX / rhs;
        result.m_dY = lhs.m_dY / rhs;
        result.m_dZ = lhs.m_dZ / rhs;
        return result;
    }

    inline A3DVector3dData &operator/=(A3DVector3dData &lhs, double const &rhs ) {
        lhs.m_dX /= rhs;
        lhs.m_dY /= rhs;
        lhs.m_dZ /= rhs;
        return lhs;
    }
    
    inline A3DVector3dData cross( A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = lhs.m_dY * rhs.m_dZ - lhs.m_dZ * rhs.m_dY;
        result.m_dY = lhs.m_dZ * rhs.m_dX - lhs.m_dX * rhs.m_dZ;
        result.m_dZ = lhs.m_dX * rhs.m_dY - lhs.m_dY * rhs.m_dX;
        return result;
    }

    inline double dot( A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        return lhs.m_dX * rhs.m_dX + lhs.m_dY * rhs.m_dY + lhs.m_dZ * rhs.m_dZ;
    }
    
    inline double length2( A3DVector3dData const &v ) {
        return dot( v, v );
    }
    
    inline double length( A3DVector3dData const &v ) {
        return sqrt( length2( v ) );
    }
    
    inline A3DVector3dData& normalize( A3DVector3dData &v ) {
        v /= length( v );
        return v;
    }
    
    inline A3DVector3dData normalized( A3DVector3dData const &v ) {
        auto result = v;
        return normalize( result );
    }

    inline bool operator==(A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        static auto const RESOLUTION = 1.e-9;
        return length( lhs - rhs ) < RESOLUTION;
    }

    inline bool operator!=(A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        static auto const RESOLUTION = 1.e-9;
        return length( lhs - rhs ) > RESOLUTION;
    }
    
    inline A3DBoundingBoxData &include( A3DBoundingBoxData &bb, A3DVector3dData const &pt ) {
        using namespace std;
        bb.m_sMin.m_dX = min( bb.m_sMin.m_dX, pt.m_dX );
        bb.m_sMin.m_dY = min( bb.m_sMin.m_dY, pt.m_dY );
        bb.m_sMin.m_dZ = min( bb.m_sMin.m_dZ, pt.m_dZ );

        bb.m_sMax.m_dX = max( bb.m_sMax.m_dX, pt.m_dX );
        bb.m_sMax.m_dY = max( bb.m_sMax.m_dY, pt.m_dY );
        bb.m_sMax.m_dZ = max( bb.m_sMax.m_dZ, pt.m_dZ );
        
        return bb;
    }
    
    inline A3DVector3dData center( A3DBoundingBoxData const &bb ) {
        return bb.m_sMin + (bb.m_sMax - bb.m_sMin) * 0.5;
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

