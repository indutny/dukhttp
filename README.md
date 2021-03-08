# dukhttp

## Build instructions

```sh
brew install cmake
mkdir -p build
cd build
cmake ..
cmake --build . -j9
cd -
```

## Run instructions

```sh
./build/dukhttp ./examples/handler.js
```

## Benchmarks

```sh
% wrk -c 100 -t 8 -d 300 http://127.0.0.1:6007/
Running 5m test @ http://127.0.0.1:6007/
  8 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   595.97us  384.36us  38.82ms   96.87%
    Req/Sec    20.34k     1.37k   22.42k    84.25%
  26312193 requests in 2.71m, 1.69GB read
Requests/sec: 161918.10
Transfer/sec:     10.65MB
```

#### LICENSE

This software is licensed under the MIT License.

Copyright Fedor Indutny, 2021.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.
