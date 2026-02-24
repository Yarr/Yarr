# Scan Loops

The core of the scan engine is the scan loop action.

These form a hierarchical scan structure, with the actions of inner loops
proceeding at least once for an iteration of the outer loop.

There are some special loops. The job of the data loop is to provide data
to the data processor, which feeds the histogrammers.

The trigger loop is provided by the FrontEnd specific library and allows
building and executing a trigger loop which generates data.

## StdDataGatherer

This loop gathers data until a time limit is reached, or until a signal is
received.

## StdDataLoop

The normal data loop collects data until the appropriate number of events
have been collected.

## StdParameterLoop

The parameter loop provides a way to iterate over any variable.

## StdRepeater

The repeater repeats its contents a fixed number of times. The main use case
is for time evolution.
