
How use the Exchange Toolkit
============================

To use the ExchangeToolkit in your project, simply add the header `ExchangeToolkit.h` to your source code. If you intend to use the [Eigen Bridge](https://techsoft3d.github.io/ExchangeToolkit/group__eigen__bridge.html), copy `ExchangeEigenBridge.h` as well. The Eigen Bridge is optional, and requires [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page). 

API Reference
=============
The ExchangeToolkit API documentation can be found [here](https://techsoft3d.github.io/ExchangeToolkit/)


How to Build the Samples (optional)
-----------------------------------------
* Prerequisites
	* For Windows, you'll need Visual Studio 2015.
	* For Mac/Linux, you'll need qmake, which is part of Qt. Any modern version will do.

* Building on Windows
  * Edit the file ExchangeToolkit/build/VisualStudio/devenv.bat
  * Update the HOOPS_EXCHANGE_DIR variable so it points to your installation of HOOPS Exchange.
     ** BE SURE TO USE FORWARD SLASHES
  * Save, and open the .bat file
  * Build the solution

* Building on Mac/Linux
  * Edit the file ExchangeToolkit/build/qmake/config.pri
  * Update the HOOPS_EXCHANGE_DIR and HOOPS_VISUALIZE_DIR variables to point to your installations
     ** BE SURE TO USE FORWARD SLASHES
  * Save the file and run qmake
  * Run make


The [ExchangeToolkit/examples](https://github.com/techsoft3d/ExchangeToolkit/tree/master/examples) folder contains a number of projects that illustrate common use cases.

The [ExchangeToolkit/ExchangeToolkit/include](https://github.com/techsoft3d/ExchangeToolkit/tree/master/ExchangeToolkit/include) folder contains the toolkit itself.


