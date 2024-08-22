# PNG Radius Image Strip
Creates a transparent radius around a given PNG image.

# Usage
## Compile
Use [bash scripts](./scripts/) to compile the project.

```sh
# Using the compile script, creates an "app" binary
$ ./scripts/build.sh ./main.cc

# You can strip out debug symbols if you'd like
$ strip ./app
```

## Run
After compiling, use the `-h` flag to show available tool options.

Basic functionality of the tool consist of supplying the `-r` radius flag and path to the image being modified.
```sh
$ ./app -r 10 ./path_to_image.png
Image Parsed:
  - Color type = 2
  - Bit depth  = 8
  - Channels   = 4
  - Height     = 604
  - Width      = 814
Wrote new image to 'out.png'
```

# License
Licensed under [MIT](./LICENSE.md).
