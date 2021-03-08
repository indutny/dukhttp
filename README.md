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
% wrk --latency -c 100 -t 8 -d 60 http://127.0.0.1:6007/
Running 1m test @ http://127.0.0.1:6007/
  8 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   513.90us  482.86us  36.40ms   99.21%
    Req/Sec    23.65k     1.26k   25.71k    80.37%
  Latency Distribution
     50%  491.00us
     75%  514.00us
     90%  541.00us
     99%    0.98ms
  11312516 requests in 1.00m, 668.88MB read
Requests/sec: 188223.65
Transfer/sec:     11.13MB
```

Peak memory usage: 12MB.

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
