This folder contains two projects:

ExchangeToolkit - This provides a wrapper on HOOPS Exchange to more easily access product structure
and more. 

viz - This is a "test driver" application that uses ExchangeToolkit, HOOPS Visualize and Qt.


REQUIRED TOOLKITS
-----------------
Qt 5.*
Eigen 3.*
HOOPS Exchange
HOOPS Visualize

STEP TO BUILD
-------------
1. Open a development command prompt. On Windows, make sure it's a Visual Studio command prompt.
2. Add Qt to the environment: PATH, QTDIR. On Windows simply execute <qt>/bin/qtenv2.bat.
3. Edit config.pri and update the paths for Eigen, Exchange and Viz.
4. On Windows, add Vizualize's debug dll folder to your PATH.
5. Generate Makefiles (qmake -r) or Mac/Windows workspaces. For windows: qmake -r -tp vc. For Mac: qmake -r -spec macx-xcode.
6. Build by running make, nmake (Makefiles), or open the Mac/Windows workspaces to build and run.


