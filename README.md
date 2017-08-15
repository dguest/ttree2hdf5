Example HDF5 Writer
===================


Run `bin/hdf-tuple-from-root` to convert your root tree to an HDF5
file. There's still A bit of work to do of course:

 - Merge with the other [root to hdf5 converter][1] I made for histograms.

 - This should be extended to support 2d arrays, like we use for
   tracks in jets.

[1]: https://github.com/dguest/th2hdf5
