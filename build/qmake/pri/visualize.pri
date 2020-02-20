isEmpty( HOOPS_VISUALIZE_PRI_INCLUDED) {
    isEmpty(HOOPS_VISUALIZE_PATH) {
		isEmpty(TS3D_PATH) : error("visualize.pri: varaible TS3D_PATH must be defined as the folder where visualize is installed")
		isEmpty(HOOPS_VISUALIZE_MAJOR_VERSION) : error("visualize.pri: variabel HOOPS_VISUALIZE_MAJOR_VERSION must be defined as the major version of the visualize product, ie 2018")

		!exists($${TS3D_PATH}) : error("visualize.pri: variable TS3D_PATH points to $${TS3D_PATH} which does not exist.")

		HOOPS_VISUALIZE_PATH = $${TS3D_PATH}/HOOPS_Visualize_$${HOOPS_VISUALIZE_MAJOR_VERSION}

		!isEmpty(HOOPS_VISUALIZE_SERVICE_PACK) {
			HOOPS_VISUALIZE_PATH = $${HOOPS_VISUALIZE_PATH}_SP$${HOOPS_VISUALIZE_SERVICE_PACK}
		}

		!isEmpty(HOOPS_VISUALIZE_UPDATE) {
			HOOPS_VISUALIZE_PATH = $${HOOPS_VISUALIZE_PATH}_U$${HOOPS_VISUALIZE_UPDATE}
		}
		!exists($${HOOPS_VISUALIZE_PATH}) : error("visualize.pri: derived HOOPS Visualize installation directory ($${HOOPS_VISUALIZE_PATH}) does not exist.")
	}
    message(Using HOOPS Visualize: $${HOOPS_VISUALIZE_PATH})

	INCLUDEPATH += $${HOOPS_VISUALIZE_PATH}/include

	debug:D=d
	release:D=

	win32 {
        platform = win64_v140$${D}
		HOOPS_VISUALIZE_LIB_PATH = $${HOOPS_VISUALIZE_PATH}/lib/$${platform}
	    files = \
				hps_core \	
				hps_sprk \
				hps_sprk_ops

		for(file, files) {
			LIBS *= $${HOOPS_VISUALIZE_LIB_PATH}/$${file}.lib
		}
	}
	
	linux {
		platform = linux64
		HOOPS_VISUALIZE_LIB_PATH = $${HOOPS_VISUALIZE_PATH}/bin/$${platform}
		files = \
			hps_core \	
			hps_sprk \
			hps_sprk_ops

		LIBS *= -L$${HOOPS_VISUALIZE_LIB_PATH} -Wl,-rpath,$$HOOPS_VISUALIZE_LIB_PATH

		for(file, files) {
			LIBS *= -l$${file}
		}
	}

	macos {
		platform = osx
		HOOPS_VISUALIZE_LIB_PATH = $${HOOPS_VISUALIZE_PATH}/bin/$${platform}
		files = \
			hps_core \	
			hps_sprk \
			hps_sprk_ops

		LIBS *= -L$${HOOPS_VISUALIZE_LIB_PATH} -Wl,-rpath,$$HOOPS_VISUALIZE_LIB_PATH

		for(file, files) {
			LIBS *= -l$${file}
		}

	}
	
	!isEmpty( HOOPS_EXCHANGE_PRI_INCLUDED ) {
		message("Using HPS Exchange sprocket")
		DEFINES *= USING_EXCHANGE
		win32 {
			LIBS *= $${HOOPS_VISUALIZE_LIB_PATH}/hps_sprk_exchange.lib
		}

		linux|macx {
			LIBS *= -lhps_sprk_exchange
		}
	} else {
	    message("Not using HPS Exchange sprocket")
	}
	HOOPS_VISUALIZE_PRI_INCLUDED = 1
}	
