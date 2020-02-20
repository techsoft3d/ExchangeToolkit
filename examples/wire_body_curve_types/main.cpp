#define INITIALIZE_A3D_API
#include "A3DSDKIncludes.h"

#include <locale>
#include <codecvt>
#include <string>
#include <fstream>
#include <regex>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include "ExchangeToolkit.h"

#define xstr(s) __str(s)
#define __str(s) #s

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

int main( int argc, char *argv[] ) {
	auto usage = []{
		std::cerr << "Usage: wire_body_curve_types <input file>" << std::endl;
		std::cerr << "  <input file>  - specifies a the files to be read using HOOPS Exchange" << std::endl;
		std::cerr << std::endl;
		std::cerr << "This application reads the specified file then iterates over each wire" << std::endl;
		std::cerr << "and prints some basic information about the underlying curve." << std::endl;
	};
    
	if( argc != 2 ) {
		usage();
		return -1;
	}

	std::string const input_file = argv[argc-1];
	std::cout << "Input file: " << input_file << std::endl;
    
	std::string const exchange_path = xstr(HOOPS_EXCHANGE_PATH);
#ifdef __MACH__
	auto const lib_path = exchange_path + "/bin/osx64";
	A3DSDKHOOPSExchangeLoader loader( lib_path.c_str()  );
#elif __linux__
	auto const lib_path = exchange_path + "/bin/linux64";
	A3DSDKHOOPSExchangeLoader loader( lib_path.c_str() );
#else
	auto const lib_path = exchange_path + "/bin/win64";
	A3DSDKHOOPSExchangeLoader loader( converter.from_bytes( lib_path ).c_str() );
#endif
    
	if(! loader.m_bSDKLoaded ) {
		std::cerr << "Failed to load Exchange." << std::endl;
		std::cerr << "Tried: " << lib_path << std::endl;
		return -1;
	}
	std::cout << "Exchange: " << exchange_path << std::endl;

	A3DImport i( input_file.c_str() );
	i.m_sLoadData.m_sGeneral.m_bReadSolids = true;
	i.m_sLoadData.m_sGeneral.m_bReadWireframes = true;
	i.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomOnly;

	loader.Import( i );
	if( nullptr == loader.m_psModelFile ) {
		std::cerr << "The specified file could not be loaded." << std::endl;
		return -1;
	}
    
    //! [Print wire body curve types]
	auto const wire_edges = ts3d::getLeafInstances( loader.m_psModelFile, kA3DTypeTopoWireEdge);
	std::cout << "There are " << wire_edges.size() << " single wire edges." << std::endl;
	for( auto const we : wire_edges ) {
		ts3d::A3DTopoWireEdgeWrapper d( we.back() );
		ts3d::Instance i({ d->m_p3dCurve });
		std::cout << "Curve: " << i.getType();
		if (d->m_bHasTrimDomain) {
			std::cout << " [" << d->m_sInterval.m_dMin << ", " << d->m_sInterval.m_dMax << "]";
		}
		std::cout << std::endl;
		switch (i.leafType()) {
		case kA3DTypeCrvPolyLine:
		{
			ts3d::A3DCrvPolyLineWrapper curve_data(i.leaf());
			std::cout << curve_data->m_uiSize << " pts" << std::endl;
		}
		break;
		case kA3DTypeCrvCircle:
		{
			ts3d::A3DCrvCircleWrapper curve_data(i.leaf());
			auto const &o = curve_data->m_sTrsf.m_sOrigin;
			std::cout << "Center (" << o.m_dX << ", " << o.m_dY << ", " << o.m_dZ << ")" << std::endl;
			std::cout << "Radius: " << curve_data->m_dRadius << std::endl;
		}
		break;
		case kA3DTypeCrvEllipse:
		{
			ts3d::A3DCrvEllipseWrapper curve_data(i.leaf());
			auto const &o = curve_data->m_sTrsf.m_sOrigin;
			std::cout << "Center (" << o.m_dX << ", " << o.m_dY << ", " << o.m_dZ << ")" << std::endl;
			std::cout << "X Radius: " << curve_data->m_dXRadius << std::endl;
			std::cout << "Y Radius: " << curve_data->m_dYRadius << std::endl;
		}
		break;
		case kA3DTypeCrvLine:
		{
			ts3d::A3DCrvLineWrapper curve_data(i.leaf());
			auto const &o = curve_data->m_sTrsf.m_sOrigin;
			auto const &x = curve_data->m_sTrsf.m_sXVector;
			auto const &y = curve_data->m_sTrsf.m_sYVector;
			std::cout << "Origin (" << o.m_dX << ", " << o.m_dY << ", " << o.m_dZ << ")" << std::endl;
			std::cout << "X (" << x.m_dX << ", " << x.m_dY << ", " << x.m_dZ << ")" << std::endl;
			std::cout << "Y (" << y.m_dX << ", " << y.m_dY << ", " << y.m_dZ << ")" << std::endl;
		}
			break;
		}
	}
    //! [Print wire body curve types]

	return 0;
}
