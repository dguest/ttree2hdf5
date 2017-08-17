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
ttree2hdf5 <root-file> <output-file> [dims..]
```

where the `[dims..]` argument can have up to two integers.

To Do
-----

Other things I might do sometime:

 - Add a regex to filter branches
 - Allow grouping of different branches into different output datasets
 - Give the option to supply a TCut string to skim the ntuple
 - Merge with the other [root to hdf5 converter][2] I made for histograms.

If you want these things, or anything else, [file an issue][1].

[1]: https://github.com/dguest/ttree2hdf5/issues
[2]: https://github.com/dguest/th2hdf5
