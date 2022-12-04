# ReadSave
 Library for reading IDL save file variables

 With dockers, try something like this from a directory containing a save file `filename`:

 `docker run --rm -v `pwd`:/files johnathanburchill/readsav:latest files/filename.sav --variable-summary --variable=skymap.generation_info`
