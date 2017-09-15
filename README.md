Root to HDF5 Tree Converter
===========================

Run `bin/ttree2hdf5` to convert your root tree to an HDF5 file.

Branches that contain basic types (one value per entry in the tree)
are stored as a 1D array of compound data type. Branches that store
`vector<T>` are stored in a 2D array, while branches of type
`vector<vector<T> >` are stored in a 3D array. If you want more
dimensions feel free to [file an issue][1].

You can specify the maximum dimensions of the array with

```
ttree2hdf5 <root-file> -o <output-file> -l [dims..]
```

where the `[dims..]` argument can have up to two integers.

For more options check `ttree2hdf5 -h`.

Setup
-----

The makefile will try to find HDF5 (C++ interfaces required), boost
(for `program_options`) and ROOT. If these are available on your
system you can install with make.

### ATLAS + lxplus Only ###

To install on lxplus we also include a CMake file. Run

```
source setup-atlas-lxplus.sh
mkdir build
cd build
cmake ..
make -j 4
```

**WARNING:** we're currently using a custom installation of HDF5, see the issue here: https://sft.its.cern.ch/jira/browse/SPI-984


Hacking This Code
-----------------

I've _tried_ to make this code as modular and straightforward as
possible. Of course if anything is unclear you should
[file an issue][1].

The main algorithm `ttree2hdf5` is a thin wrapper on
`copy_root_tree`. Reading the code in `src/copy_root_tree.cxx` should
be a decent introduction to the higher level classes you'll need to
interact with.

As for compiling: anything in `src/` which starts with `ttree2hdf*`
will compile into an executable by default, whereas anything in `src/`
will compile and link to the executables by default.

To Do
-----

Other things I might do sometime:

 - Allow grouping of different branches into different output datasets
 - Give the option to supply a TCut string to skim the ntuple
 - Merge with the other [root to hdf5 converter][2] I made for histograms.

If you want these things, or anything else, [file an issue][1].

[1]: https://github.com/dguest/ttree2hdf5/issues
[2]: https://github.com/dguest/th2hdf5
