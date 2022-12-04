# ReadSave
 Library for reading IDL save file variables

 With docker, try something like this from a directory containing a save file `filename`:

 ``docker run --rm -v `pwd`:/files johnathanburchill/readsav:latest files/filename.sav --variable-summary``
 
 Play with a [save file from the UCalgary Auroral Imaging Group](https://data.phys.ucalgary.ca/sort_by_project/THEMIS/asi/skymaps/rank/rank_20130107/themis_skymap_rank_20130107-%2B_vXX.sav).
