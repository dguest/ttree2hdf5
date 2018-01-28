Root to HDF5 Tree Converter
===========================

Run `bin/ttree2hdf5` to convert your root tree to an HDF5 file.

Branches that contain basic types (one value per entry in the tree)
are stored as a 1D array of compound data type. Branches that store
`vector<T>` are stored in a 2D array, while branches of type
`vector<vector<T> >` are stored in a 3D array. If you want more
dimensions feel free to [file an issue][1].

Note that **we only support the types listed above**. The purpose of
this package is _not_ to provide a generic converter from ROOT to
HDF5, but rather to convert simple ROOT files to a more widely
supported data format.

Output Format
-------------

You can specify the maximum dimensions of the HDF5 array with

```
ttree2hdf5 <root-file> -o <output-file> -l [dims..]
```

where the `[dims..]` argument can have up to two integers.

You can also filter branches with the `--branch-regex` option.

For more options check `ttree2hdf5 -h`.

Setup
=====

The dependencies are HDF5 (C++ interfaces required), boost (for
`program_options`) and ROOT.

This version of the package is meant for inclusion in ATLAS code, so
the setup follows the standard setup for ATLAS releases. For the
package this is based off (which can compile independently) see here:

https://github.com/dguest/ttree2hdf5


Hacking This Code
=================

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
