//
//  main.cpp
//  ExchangeToolkit
//
//  Created by Brad Flubacher on 4/1/19.
//  Copyright © 2019 Brad Flubacher. All rights reserved.
//
#define INITIALIZE_A3D_API
#include <A3DSDKIncludes.h>

#include "HPSWidget.h"
#include "ExchangeHPSBridge.h"
#include "ExchangeToolkit.h"

#define xstr(s) str(s)
#define str(s) #s

int main(int argc, char * argv[]) {
    QString const exchange_path = xstr(HOOPS_EXCHANGE_PATH);
#ifdef __MACH__
    auto const lib_path = exchange_path + "/bin/osx64";
    A3DSDKHOOPSExchangeLoader loader( qPrintable( lib_path ) );
#elif __linux__
    auto const lib_path = exchange_path + "/bin/linux64";
    A3DSDKHOOPSExchangeLoader loader( qPrintable( lib_path ) );
#else
    auto const lib_path = exchange_path + "/bin/win64";
    A3DSDKHOOPSExchangeLoader loader( lib_path.toStdWString().c_str() );
#endif
    
    if(! loader.m_bSDKLoaded ) {
        return -1;
    }

    
    
//    auto const default_file_to_load = "/opt/local/ts3d/HOOPS_Exchange_2019_SP1/samples/data/catiaV5/CV5_Aquo_Bottle/_Aquo Bottle.CATProduct";
//    auto const default_file_to_load = "/opt/local/ts3d/HOOPS_Exchange_2019/samples/data/catiaV5/CV5_Micro_engine/_micro engine.CATProduct";
//    auto const default_file_to_load = "/opt/local/ts3d/Partners/maplesoft/Arm2 parts.stl";
//    auto const default_file_to_load = "/opt/local/ts3d/HOOPS_Exchange_2019/samples/data/catiaV5/CV5_Micro_engine/housing back.CATPart";
//    auto const default_file_to_load = "/opt/local/ts3d/HOOPS_Exchange_2019/samples/data/prc/__drill.prc";
//    auto const default_file_to_load = "/opt/local/ts3d/DataSets/acis/block.sat";
//    auto const default_file_to_load = "/opt/local/ts3d/HOOPS_Exchange_2019/samples/data/solidworks/SLW_Diskbrakeassembly/_DiskBrakeAssembly-01FINAL.SLDASM";
//    auto const default_file_to_load = "/opt/local/ts3d/HOOPS_Exchange_2019/samples/data/solidworks/SLW_Diskbrakeassembly/boostercover.SLDPRT";
//    auto const default_file_to_load = "/opt/local/ts3d/HOOPS_Exchange_2019/samples/data/solidworks/SLW_Diskbrakeassembly/brakepad.SLDPRT";
//    auto const default_file_to_load = "/opt/local/ts3d/Partners/BackToCAD/metal-u-loop.pdf";
//    auto const default_file_to_load = "/media/psf/ts3d/HOOPS_Exchange_2019/samples/data/catiaV5/CV5_Aquo_Bottle/_Aquo Bottle.CATProduct";
    auto const default_file_to_load = "/opt/local/ts3d/HOOPS_Exchange_2019_SP1/samples/data/prc/helloworld.prc";
    auto const fn = argc > 1 ? argv[1] : default_file_to_load;
    A3DImport i( fn );
    i.m_sLoadData.m_sGeneral.m_bReadSolids = true;
    i.m_sLoadData.m_sGeneral.m_bReadWireframes = true;
    i.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomAndTess;
    loader.Import( i );
    if( nullptr == loader.m_psModelFile ) {
        std::cout << "The specified file was not loaded: " << fn << std::endl;
        std::cout << "Specify the file to load as the argument, or update hard coded test file paths in main.cpp." << std::endl;
        return -1;
    }
    
    
    HPS::World world( HOOPS_LICENSE );

    QApplication app( argc, argv );

    QMainWindow *mainWindow = new QMainWindow();
    ts3d::HPSWidget *hps_widget = new ts3d::HPSWidget( mainWindow );
    mainWindow->setCentralWidget( hps_widget );
    mainWindow->show();

    auto model = HPS::Factory::CreateModel();
    hps_widget->getView().AttachModel( model );
    
    HPS::DistantLightKit light;
    light.SetDirection(HPS::Vector(0, 0, -1.5f));
    light.SetColor( HPS::RGBAColor( 0.75, 0.75, 0.75, 1. ) );
    light.SetCameraRelative(true);

    hps_widget->getView().GetSegmentKey().InsertDistantLight(light);
    hps_widget->getView().GetSegmentKey().GetVisibilityControl().SetLines( true );
    hps_widget->getView().GetSegmentKey().GetLineAttributeControl().SetWeight( 1. );
    hps_widget->getView().GetSegmentKey().GetMaterialMappingControl().SetLineColor( HPS::RGBAColor::Black() );
    
    auto const unique_part_defs = ts3d::getUniqueParts( loader.m_psModelFile );
    QHash< A3DEntity*, HPS::SegmentKey > hps_keys;
    for(auto part : unique_part_defs ) {
        hps_keys.unite( ts3d::createSegmentForPartDefinition( part ) );
    }

    auto const render_instances = ts3d::getLeafInstances( loader.m_psModelFile, kA3DTypeRiRepresentationItem );
    for( auto render_instance : render_instances ) {
        auto hps_key = hps_keys.find( render_instance.back() );
        if( std::end( hps_keys ) == hps_key ) {
            continue;
        }
        
        auto k = model.GetSegmentKey().Subsegment();
        k.IncludeSegment( hps_key.value() );
        ts3d::Instance instance( render_instance );

        auto const matrix = instance.getNetMatrix();
        HPS::MatrixKit matrix_kit;
        for( auto i = 0u; i < 4u; ++i ) {
            for( auto j = 0u; j < 4u; ++j ) {
                matrix_kit.SetElement( i, j, matrix(j, i) );
            }
        }
        k.SetModellingMatrix( matrix_kit );

        auto const net_style = instance.getNetStyle();
        if( (net_style.m_bMaterial && A3D_DEFAULT_STYLE_INDEX != net_style.m_uiRgbColorIndex) || A3D_DEFAULT_COLOR_INDEX != net_style.m_uiRgbColorIndex ) {
            auto const material_kit = ts3d::getMaterialKit( net_style );
            k.GetMaterialMappingControl().SetFaceMaterial( material_kit ).SetEdgeMaterial( material_kit ).SetVertexMaterial( material_kit );
        }
    }
    
    model.GetSegmentKey().GetCullingControl().SetBackFace( true );
    
    HPS::CameraKit ck;
    hps_widget->getView().ComputeFitWorldCamera( ck );
    hps_widget->getView().SmoothTransition( ck );

    return app.exec();
}
