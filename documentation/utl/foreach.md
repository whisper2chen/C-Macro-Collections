# foreach.md

For-Each macros. Since doing a for loop with iterators can be a lot to write, these macros solve that problem. There are two of them:

* `CMC_FOREACH` - Goes from the beginning of a Collection to the end
* `CMC_FOREAC_REV` - Goes from the end of a Collection to the beginning

## Parameters

* `PFX` - Functions prefix
* `SNAME` - Struct name
* `ITERNAME` - Iterator variable name
* `TARGET` - Target variable
