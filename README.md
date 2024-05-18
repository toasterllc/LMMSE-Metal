# LMMSE-Metal

This repo implements [Linear Minimum Mean Square-Error Estimation (LMMSE)](https://www4.comp.polyu.edu.hk/~cslzhang/paper/LMMSEdemosaicing.pdf) with C++ and Metal shaders. It's a translation of [Pascal Getreuer](https://www.ipol.im/pub/art/2011/g_zwld/)'s implementation to Metal.

This repo is used by the [Photon](https://toaster.llc/photon) camera to implement debayering.

See [this blog post](http://toaster.llc/blog/image-pipeline) for more info about Photon's image pipeline.



## Performance

The average time to process a single image (with the included example project) is ~1 ms on an Apple M3 MacBook Pro.


## Example

See the `Example` directory for an example project that runs LMMSE on some static images.
