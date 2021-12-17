# atomicz

This package exposes the C++ functions `std::atomic::load`, `std::atomic::store`, and `std::atomic::exchange` to Python.

## Installation

```
pip3 install -U atomicz
```

## Usage

```
import numpy as np
import atomicz
buf = bytearray(b'foo')

>>> atomicz.exchange(np.array(255, dtype=np.uint8), buf)
102
>>> buf
bytearray(b'\xffoo')
```
