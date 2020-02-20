#!/bin/bash
scp -r ../../ExchangeToolkit/doc/html little:docs-test/ExchangeToolkit.new
ssh little "mv docs-test/ExchangeToolkit docs-test/ExchangeToolkit.old && mv docs-test/ExchangeToolkit.new docs-test/ExchangeToolkit && rm -rf docs-test/ExchangeToolkit.old"
