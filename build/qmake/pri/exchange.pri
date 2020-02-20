isEmpty( HOOPS_EXCHANGE_PRI_INCLUDED) {
	isEmpty( HOOPS_EXCHANGE_PATH ) {
		isEmpty(TS3D_PATH) : error("exchange.pri: varaible TS3D_PATH must be defined as the folder where exchange is installed")
		isEmpty(HOOPS_EXCHANGE_MAJOR_VERSION) : error("exchange.pri: variabel HOOPS_EXCHANGE_MAJOR_VERSION must be defined as the major version of the exchange product, ie 2018")

		!exists($${TS3D_PATH}) : error("exchange.pri: variable TS3D_PATH points to $${TS3D_PATH} which does not exist.")

		HOOPS_EXCHANGE_PATH = $${TS3D_PATH}/HOOPS_Exchange_Publish_$${HOOPS_EXCHANGE_MAJOR_VERSION}

		!isEmpty(HOOPS_EXCHANGE_SERVICE_PACK) {
			HOOPS_EXCHANGE_PATH = $${HOOPS_EXCHANGE_PATH}_SP$${HOOPS_EXCHANGE_SERVICE_PACK}
		}

		!isEmpty(HOOPS_EXCHANGE_UPDATE) {
			HOOPS_EXCHANGE_PATH = $${HOOPS_EXCHANGE_PATH}_U$${HOOPS_EXCHANGE_UPDATE}
		}

		!exists($${HOOPS_EXCHANGE_PATH}) : error("exchange.pri: derived HOOPS Exchange installation directory ($${HOOPS_EXCHANGE_PATH}) does not exist.")
	}
    message(Using HOOPS Exchange: $${HOOPS_EXCHANGE_PATH})

	INCLUDEPATH += $${HOOPS_EXCHANGE_PATH}/include

	DEFINES += HOOPS_EXCHANGE_PATH="$${HOOPS_EXCHANGE_PATH}" _UNICODE

    linux {
        LIBS *= -ldl
    }

	HOOPS_EXCHANGE_PRI_INCLUDED = 1


}	
