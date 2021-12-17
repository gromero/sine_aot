To build and flash the `sine` inference model using the AOTExecutor:

1. Clone this repo.

2. Build and flash:

```sh
$ mkdir ./build && cd build
$ cmake .. -DBOARD=stm32f746g_disco
$ make
$ make flash
```

Then open the serial console on `minicom`, etc (e.g. `/dev/ttyACM0`) and you
must see:

```
Hit 'X' to do inference:
```

On pressing `X` you run the sine model to infer `sin(1.0)` as defined in
`src/input_data.h`:

```
Hit 'X' to do inference:
Calling TVMInfer()...
0.807911, 0.028069 ms
```

The sine inference model was compiled using `tvmc compile`:

```sh
$ tvmc compile --target="c -keys=cpu -link-params=0 -march=armv7e-m -model=stm32f746xx -system-lib=0" --target-c-mcpu=cortex-m7 --runtime=crt --executor=aot --executor-aot-interface-api=c --executor-aot-unpacked-api=1 --output-format mlf --pass-config tir.disable_vectorize=1 --disabled-pass="AlterOpLayout" sine_model.tflite --output sine.tar
```

... and was taken from:

```sh
$ wget https://people.linaro.org/~tom.gall/sine_model.tflite
```
