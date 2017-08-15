Example HDF5 Writer
===================

See `src/hdf-tuple-main.cxx` for an example of how this works. There's
still A bit of work to do of course:

 - I'd like to make a root tree converter. This could be similar to
   (or part of) the [root to hdf5 converter][1] I made for histograms.

 - This should be extended to support 2d arrays, like we use for
   tracks in jets.

[1]: https://github.com/dguest/th2hdf5
