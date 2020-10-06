#!/bin/sh
cd build/doc
doxygen ExchangeToolkit.config
touch .nojekyll
cd ../.. 
