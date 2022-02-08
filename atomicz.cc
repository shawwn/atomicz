/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
/* Modified by martin.croome@greenwaves-technologies.com - Modifications to allow a standalone build
   and remove requirements for pybind11 and other tensorflow dependencies
   Add support for scalar operations and python numeric types
*/

#include <iostream>
#include <array>
#include <locale>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
// #define DEBUG_CALLS

#include <Python.h>
#include <cinttypes>
#include <vector>
#include <atomic>
#ifdef DEBUG_CALLS
#include <iostream>
#endif
#include <fenv.h>


namespace greenwaves
{
	// Initializes the module.
	bool Initialize()
	{
    bool ok = true;
		return ok;
	}

	static PyObject *AtomiczError;

  template<typename T>
  void store(void* ptr, const T val, std::memory_order order = std::memory_order_seq_cst)
  {
    static_cast<std::atomic<T>*>(ptr)->store(val, order);
  }

  template<typename T>
  T load(const void* ptr, std::memory_order order = std::memory_order_seq_cst)
  {
    return static_cast<const std::atomic<T>*>(ptr)->load(order);
  }

  template<typename T>
  T exchange(void* ptr, const T val, std::memory_order order = std::memory_order_seq_cst)
  {
    return static_cast<std::atomic<T>*>(ptr)->exchange(val, order);
  }

  template<typename T>
  T compare_exchange(void* ptr, T* expected, const T desired, std::memory_order succ = std::memory_order_seq_cst, std::memory_order fail = std::memory_order_seq_cst)
  {
    return std::atomic_compare_exchange_strong_explicit(static_cast<std::atomic<T>*>(ptr), expected, desired, succ, fail);
  }

  template<typename T>
  T fetch_add(void* ptr, const T val, std::memory_order order = std::memory_order_seq_cst)
  {
    return static_cast<std::atomic<T>*>(ptr)->fetch_add(val, order);
  }

  template<typename T>
  T fetch_sub(void* ptr, const T val, std::memory_order order = std::memory_order_seq_cst)
  {
    return static_cast<std::atomic<T>*>(ptr)->fetch_sub(val, order);
  }

  template<typename T>
  T fetch_or(void* ptr, const T val, std::memory_order order = std::memory_order_seq_cst)
  {
    return static_cast<std::atomic<T>*>(ptr)->fetch_or(val, order);
  }

  template<typename T>
  T fetch_xor(void* ptr, const T val, std::memory_order order = std::memory_order_seq_cst)
  {
    return static_cast<std::atomic<T>*>(ptr)->fetch_xor(val, order);
  }

  template<typename T>
  T fetch_and(void* ptr, const T val, std::memory_order order = std::memory_order_seq_cst)
  {
    return static_cast<std::atomic<T>*>(ptr)->fetch_and(val, order);
  }

	void do_store(const void *source_p, void *sink_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return store<unsigned char>(sink_p, *reinterpret_cast<const unsigned char*>(source_p));
      case 2: return store<unsigned short>(sink_p, *reinterpret_cast<const unsigned short*>(source_p));
      case 4: return store<unsigned int>(sink_p, *reinterpret_cast<const unsigned int*>(source_p));
      case 8: return store<unsigned long long>(sink_p, *reinterpret_cast<const unsigned long long*>(source_p));
      default:
        fprintf(stderr, "Can only exchange 1, 2, 4, or 8 bytes\n");
        return;
    }
  }

	unsigned long long do_load(const void *source_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return load<unsigned char>(source_p);
      case 2: return load<unsigned short>(source_p);
      case 4: return load<unsigned int>(source_p);
      case 8: return load<unsigned long long>(source_p);
      default:
        fprintf(stderr, "Can only exchange 1, 2, 4, or 8 bytes\n");
        return 0;
    }
  }

	unsigned long long do_exchange(const void *source_p, void *sink_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return exchange<unsigned char>(sink_p, *reinterpret_cast<const unsigned char*>(source_p));
      case 2: return exchange<unsigned short>(sink_p, *reinterpret_cast<const unsigned short*>(source_p));
      case 4: return exchange<unsigned int>(sink_p, *reinterpret_cast<const unsigned int*>(source_p));
      case 8: return exchange<unsigned long long>(sink_p, *reinterpret_cast<const unsigned long long*>(source_p));
      default:
        fprintf(stderr, "Can only exchange 1, 2, 4, or 8 bytes\n");
        return 0;
    }
  }

	unsigned long long do_compare_exchange(const void *desired_p, void* expected_p, void *sink_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return compare_exchange<unsigned char>(sink_p, reinterpret_cast<unsigned char*>(expected_p), *reinterpret_cast<const unsigned char*>(desired_p));
      case 2: return compare_exchange<unsigned short>(sink_p, reinterpret_cast<unsigned short*>(expected_p), *reinterpret_cast<const unsigned short*>(desired_p));
      case 4: return compare_exchange<unsigned int>(sink_p, reinterpret_cast<unsigned int*>(expected_p), *reinterpret_cast<const unsigned int*>(desired_p));
      case 8: return compare_exchange<unsigned long long>(sink_p, reinterpret_cast<unsigned long long*>(expected_p), *reinterpret_cast<const unsigned long long*>(desired_p));
      default:
        fprintf(stderr, "Can only compare_exchange 1, 2, 4, or 8 bytes\n");
        return 0;
    }
  }

	unsigned long long do_fetch_add(const void *source_p, void *sink_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return fetch_add<unsigned char>(sink_p, *reinterpret_cast<const unsigned char*>(source_p));
      case 2: return fetch_add<unsigned short>(sink_p, *reinterpret_cast<const unsigned short*>(source_p));
      case 4: return fetch_add<unsigned int>(sink_p, *reinterpret_cast<const unsigned int*>(source_p));
      case 8: return fetch_add<unsigned long long>(sink_p, *reinterpret_cast<const unsigned long long*>(source_p));
      default:
        fprintf(stderr, "Can only fetch_add 1, 2, 4, or 8 bytes\n");
        return 0;
    }
  }

	unsigned long long do_fetch_sub(const void *source_p, void *sink_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return fetch_sub<unsigned char>(sink_p, *reinterpret_cast<const unsigned char*>(source_p));
      case 2: return fetch_sub<unsigned short>(sink_p, *reinterpret_cast<const unsigned short*>(source_p));
      case 4: return fetch_sub<unsigned int>(sink_p, *reinterpret_cast<const unsigned int*>(source_p));
      case 8: return fetch_sub<unsigned long long>(sink_p, *reinterpret_cast<const unsigned long long*>(source_p));
      default:
        fprintf(stderr, "Can only fetch_sub 1, 2, 4, or 8 bytes\n");
        return 0;
    }
  }

	unsigned long long do_fetch_or(const void *source_p, void *sink_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return fetch_or<unsigned char>(sink_p, *reinterpret_cast<const unsigned char*>(source_p));
      case 2: return fetch_or<unsigned short>(sink_p, *reinterpret_cast<const unsigned short*>(source_p));
      case 4: return fetch_or<unsigned int>(sink_p, *reinterpret_cast<const unsigned int*>(source_p));
      case 8: return fetch_or<unsigned long long>(sink_p, *reinterpret_cast<const unsigned long long*>(source_p));
      default:
        fprintf(stderr, "Can only fetch_or 1, 2, 4, or 8 bytes\n");
        return 0;
    }
  }

	unsigned long long do_fetch_xor(const void *source_p, void *sink_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return fetch_xor<unsigned char>(sink_p, *reinterpret_cast<const unsigned char*>(source_p));
      case 2: return fetch_xor<unsigned short>(sink_p, *reinterpret_cast<const unsigned short*>(source_p));
      case 4: return fetch_xor<unsigned int>(sink_p, *reinterpret_cast<const unsigned int*>(source_p));
      case 8: return fetch_xor<unsigned long long>(sink_p, *reinterpret_cast<const unsigned long long*>(source_p));
      default:
        fprintf(stderr, "Can only fetch_xor 1, 2, 4, or 8 bytes\n");
        return 0;
    }
  }

	unsigned long long do_fetch_and(const void *source_p, void *sink_p, unsigned long long nbytes)
	{
    switch(nbytes) {
      case 1: return fetch_and<unsigned char>(sink_p, *reinterpret_cast<const unsigned char*>(source_p));
      case 2: return fetch_and<unsigned short>(sink_p, *reinterpret_cast<const unsigned short*>(source_p));
      case 4: return fetch_and<unsigned int>(sink_p, *reinterpret_cast<const unsigned int*>(source_p));
      case 8: return fetch_and<unsigned long long>(sink_p, *reinterpret_cast<const unsigned long long*>(source_p));
      default:
        fprintf(stderr, "Can only fetch_and 1, 2, 4, or 8 bytes\n");
        return 0;
    }
  }

	static PyObject *
	load_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long ret = 0;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer source_view;
		int got_source_view = 0;
		/* read the args */
		PyObject *obj_source;
		do {
      if (!PyArg_ParseTuple(args, "O", &obj_source)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_source)) {
        PyErr_SetString(AtomiczError,
            "the source object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_source, &source_view, flags) < 0) {
        except = 1; break;
      } else {
        got_source_view = 1;
      }
      /* get the element count */
      for (i = 0; i < source_view.ndim; i++) {
        nelems *= source_view.shape[i];
      }
      /* buffers should be non-null */
      if (source_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "source buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      switch (source_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only load itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      ret = do_load(source_view.buf, source_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_source_view) {
			PyBuffer_Release(&source_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return PyLong_FromLong(ret);
		}
	}

	static PyObject *
	store_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer source_view;
		Py_buffer sink_view;
		int got_source_view = 0;
		int got_sink_view = 0;
		/* read the args */
		PyObject *obj_source;
		PyObject *obj_sink;
		do {
      if (!PyArg_ParseTuple(args, "OO", &obj_sink, &obj_source)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_source)) {
        PyErr_SetString(AtomiczError,
            "the source object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_sink)) {
        PyErr_SetString(AtomiczError,
            "the sink object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_source, &source_view, flags) < 0) {
        except = 1; break;
      } else {
        got_source_view = 1;
      }
      if (PyObject_GetBuffer(obj_sink, &sink_view, flags) < 0) {
        except = 1; break;
      } else {
        got_sink_view = 1;
      }
      /* the buffers should be the same rank */
      if (std::max(1,source_view.ndim) != std::max(1,sink_view.ndim)) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same rank, got source.ndim == %d and sink.ndim == %d", source_view.ndim, sink_view.ndim);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* the buffers should be the same shape */
      for (i = 0; i < source_view.ndim; i++) {
        if (source_view.shape[i] != sink_view.shape[i]) {
          PyErr_SetString(AtomiczError,
              "the source and sink buffers should be the same shape");
          except = 1; break;
        }
        nelems *= source_view.shape[i];
      }
      if (except) {
        break;
      }
      /* the buffers should be the itemsize */
      if (source_view.itemsize != sink_view.itemsize) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same itemsize, got source.itemsize == %llu and sink.itemsize == %llu", (unsigned long long)source_view.itemsize, (unsigned long long)sink_view.itemsize);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* buffers should be non-null */
      if (source_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "source buffer is NULL");
        except = 1; break;
      }
      if (sink_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "sink buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      if (sink_view.readonly) {
        PyErr_SetString(AtomiczError, "sink buffer is readonly");
        except = 1; break;
      }
      switch (sink_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only store itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      do_store(source_view.buf, sink_view.buf, sink_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_source_view) {
			PyBuffer_Release(&source_view);
		}
		if (got_sink_view) {
			PyBuffer_Release(&sink_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return Py_BuildValue("");
		}
	}

	static PyObject *
	exchange_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long ret = 0;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer source_view;
		Py_buffer sink_view;
		int got_source_view = 0;
		int got_sink_view = 0;
		/* read the args */
		PyObject *obj_source;
		PyObject *obj_sink;
		do {
      if (!PyArg_ParseTuple(args, "OO", &obj_sink, &obj_source)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_source)) {
        PyErr_SetString(AtomiczError,
            "the source object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_sink)) {
        PyErr_SetString(AtomiczError,
            "the sink object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_source, &source_view, flags) < 0) {
        except = 1; break;
      } else {
        got_source_view = 1;
      }
      if (PyObject_GetBuffer(obj_sink, &sink_view, flags) < 0) {
        except = 1; break;
      } else {
        got_sink_view = 1;
      }
      /* the buffers should be the same rank */
      if (std::max(1,source_view.ndim) != std::max(1,sink_view.ndim)) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same rank, got source.ndim == %d and sink.ndim == %d", source_view.ndim, sink_view.ndim);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* the buffers should be the same shape */
      for (i = 0; i < source_view.ndim; i++) {
        if (source_view.shape[i] != sink_view.shape[i]) {
          PyErr_SetString(AtomiczError,
              "the source and sink buffers should be the same shape");
          except = 1; break;
        }
        nelems *= source_view.shape[i];
      }
      if (except) {
        break;
      }
      /* the buffers should be the itemsize */
      if (source_view.itemsize != sink_view.itemsize) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same itemsize, got source.itemsize == %llu and sink.itemsize == %llu", (unsigned long long)source_view.itemsize, (unsigned long long)sink_view.itemsize);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* buffers should be non-null */
      if (source_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "source buffer is NULL");
        except = 1; break;
      }
      if (sink_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "sink buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      if (sink_view.readonly) {
        PyErr_SetString(AtomiczError, "sink buffer is readonly");
        except = 1; break;
      }
      switch (sink_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only exchange itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      ret = do_exchange(source_view.buf, sink_view.buf, sink_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_source_view) {
			PyBuffer_Release(&source_view);
		}
		if (got_sink_view) {
			PyBuffer_Release(&sink_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return PyLong_FromLong(ret);
		}
	}

	static PyObject *
	compare_exchange_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long ret = 0;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer desired_view;
		Py_buffer expected_view;
		Py_buffer sink_view;
		int got_desired_view = 0;
		int got_expected_view = 0;
		int got_sink_view = 0;
		/* read the args */
		PyObject *obj_desired;
		PyObject *obj_expected;
		PyObject *obj_sink;
		do {
      if (!PyArg_ParseTuple(args, "OOO", &obj_sink, &obj_expected, &obj_desired)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_desired)) {
        PyErr_SetString(AtomiczError,
            "the desired object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_expected)) {
        PyErr_SetString(AtomiczError,
            "the expected object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_sink)) {
        PyErr_SetString(AtomiczError,
            "the sink object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_desired, &desired_view, flags) < 0) {
        except = 1; break;
      } else {
        got_desired_view = 1;
      }
      if (PyObject_GetBuffer(obj_expected, &expected_view, flags) < 0) {
        except = 1; break;
      } else {
        got_expected_view = 1;
      }
      if (PyObject_GetBuffer(obj_sink, &sink_view, flags) < 0) {
        except = 1; break;
      } else {
        got_sink_view = 1;
      }
      /* the buffers should be the same rank */
      if (std::max(1,expected_view.ndim) != std::max(1,sink_view.ndim) || std::max(1,desired_view.ndim) != std::max(1,sink_view.ndim)) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the expected, desired and sink buffers should be the same rank, got expected.ndim == %d, desired.ndim == %d, and sink.ndim == %d", expected_view.ndim, desired_view.ndim, sink_view.ndim);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* the buffers should be the same shape */
      for (i = 0; i < desired_view.ndim; i++) {
        if (expected_view.shape[i] != sink_view.shape[i] || desired_view.shape[i] != sink_view.shape[i]) {
          PyErr_SetString(AtomiczError,
              "the expected, desired, and sink buffers should be the same shape");
          except = 1; break;
        }
        nelems *= desired_view.shape[i];
      }
      if (except) {
        break;
      }
      /* the buffers should be the itemsize */
      if (expected_view.itemsize != sink_view.itemsize || desired_view.itemsize != sink_view.itemsize) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the expected, desired, and sink buffers should be the same itemsize, got expected.itemsize == %llu, desired.itemsize == %llu, and sink.itemsize == %llu", (unsigned long long)&expected_view.itemsize, (unsigned long long)desired_view.itemsize, (unsigned long long)sink_view.itemsize);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* buffers should be non-null */
      if (expected_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "expected buffer is NULL");
        except = 1; break;
      }
      if (desired_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "desired buffer is NULL");
        except = 1; break;
      }
      if (sink_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "sink buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      if (sink_view.readonly) {
        PyErr_SetString(AtomiczError, "sink buffer is readonly");
        except = 1; break;
      }
      if (expected_view.readonly) {
        PyErr_SetString(AtomiczError, "expected buffer is readonly");
        except = 1; break;
      }
      switch (sink_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only compare_exchange sink itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      switch (expected_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only compare_exchange expected itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      ret = do_compare_exchange(desired_view.buf, expected_view.buf, sink_view.buf, sink_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_desired_view) {
			PyBuffer_Release(&desired_view);
		}
		if (got_expected_view) {
			PyBuffer_Release(&expected_view);
		}
		if (got_sink_view) {
			PyBuffer_Release(&sink_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return PyLong_FromLong(ret);
		}
	}

	static PyObject *
	fetch_add_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long ret = 0;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer source_view;
		Py_buffer sink_view;
		int got_source_view = 0;
		int got_sink_view = 0;
		/* read the args */
		PyObject *obj_source;
		PyObject *obj_sink;
		do {
      if (!PyArg_ParseTuple(args, "OO", &obj_sink, &obj_source)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_source)) {
        PyErr_SetString(AtomiczError,
            "the source object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_sink)) {
        PyErr_SetString(AtomiczError,
            "the sink object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_source, &source_view, flags) < 0) {
        except = 1; break;
      } else {
        got_source_view = 1;
      }
      if (PyObject_GetBuffer(obj_sink, &sink_view, flags) < 0) {
        except = 1; break;
      } else {
        got_sink_view = 1;
      }
      /* the buffers should be the same rank */
      if (std::max(1,source_view.ndim) != std::max(1,sink_view.ndim)) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same rank, got source.ndim == %d and sink.ndim == %d", source_view.ndim, sink_view.ndim);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* the buffers should be the same shape */
      for (i = 0; i < source_view.ndim; i++) {
        if (source_view.shape[i] != sink_view.shape[i]) {
          PyErr_SetString(AtomiczError,
              "the source and sink buffers should be the same shape");
          except = 1; break;
        }
        nelems *= source_view.shape[i];
      }
      if (except) {
        break;
      }
      /* the buffers should be the itemsize */
      if (source_view.itemsize != sink_view.itemsize) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same itemsize, got source.itemsize == %llu and sink.itemsize == %llu", (unsigned long long)source_view.itemsize, (unsigned long long)sink_view.itemsize);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* buffers should be non-null */
      if (source_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "source buffer is NULL");
        except = 1; break;
      }
      if (sink_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "sink buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      if (sink_view.readonly) {
        PyErr_SetString(AtomiczError, "sink buffer is readonly");
        except = 1; break;
      }
      switch (sink_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only fetch_add itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      ret = do_fetch_add(source_view.buf, sink_view.buf, sink_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_source_view) {
			PyBuffer_Release(&source_view);
		}
		if (got_sink_view) {
			PyBuffer_Release(&sink_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return PyLong_FromLong(ret);
		}
	}

	static PyObject *
	fetch_sub_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long ret = 0;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer source_view;
		Py_buffer sink_view;
		int got_source_view = 0;
		int got_sink_view = 0;
		/* read the args */
		PyObject *obj_source;
		PyObject *obj_sink;
		do {
      if (!PyArg_ParseTuple(args, "OO", &obj_sink, &obj_source)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_source)) {
        PyErr_SetString(AtomiczError,
            "the source object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_sink)) {
        PyErr_SetString(AtomiczError,
            "the sink object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_source, &source_view, flags) < 0) {
        except = 1; break;
      } else {
        got_source_view = 1;
      }
      if (PyObject_GetBuffer(obj_sink, &sink_view, flags) < 0) {
        except = 1; break;
      } else {
        got_sink_view = 1;
      }
      /* the buffers should be the same rank */
      if (std::max(1,source_view.ndim) != std::max(1,sink_view.ndim)) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same rank, got source.ndim == %d and sink.ndim == %d", source_view.ndim, sink_view.ndim);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* the buffers should be the same shape */
      for (i = 0; i < source_view.ndim; i++) {
        if (source_view.shape[i] != sink_view.shape[i]) {
          PyErr_SetString(AtomiczError,
              "the source and sink buffers should be the same shape");
          except = 1; break;
        }
        nelems *= source_view.shape[i];
      }
      if (except) {
        break;
      }
      /* the buffers should be the itemsize */
      if (source_view.itemsize != sink_view.itemsize) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same itemsize, got source.itemsize == %llu and sink.itemsize == %llu", (unsigned long long)source_view.itemsize, (unsigned long long)sink_view.itemsize);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* buffers should be non-null */
      if (source_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "source buffer is NULL");
        except = 1; break;
      }
      if (sink_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "sink buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      if (sink_view.readonly) {
        PyErr_SetString(AtomiczError, "sink buffer is readonly");
        except = 1; break;
      }
      switch (sink_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only fetch_sub itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      ret = do_fetch_sub(source_view.buf, sink_view.buf, sink_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_source_view) {
			PyBuffer_Release(&source_view);
		}
		if (got_sink_view) {
			PyBuffer_Release(&sink_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return PyLong_FromLong(ret);
		}
	}

	static PyObject *
	fetch_or_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long ret = 0;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer source_view;
		Py_buffer sink_view;
		int got_source_view = 0;
		int got_sink_view = 0;
		/* read the args */
		PyObject *obj_source;
		PyObject *obj_sink;
		do {
      if (!PyArg_ParseTuple(args, "OO", &obj_sink, &obj_source)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_source)) {
        PyErr_SetString(AtomiczError,
            "the source object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_sink)) {
        PyErr_SetString(AtomiczError,
            "the sink object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_source, &source_view, flags) < 0) {
        except = 1; break;
      } else {
        got_source_view = 1;
      }
      if (PyObject_GetBuffer(obj_sink, &sink_view, flags) < 0) {
        except = 1; break;
      } else {
        got_sink_view = 1;
      }
      /* the buffers should be the same rank */
      if (std::max(1,source_view.ndim) != std::max(1,sink_view.ndim)) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same rank, got source.ndim == %d and sink.ndim == %d", source_view.ndim, sink_view.ndim);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* the buffers should be the same shape */
      for (i = 0; i < source_view.ndim; i++) {
        if (source_view.shape[i] != sink_view.shape[i]) {
          PyErr_SetString(AtomiczError,
              "the source and sink buffers should be the same shape");
          except = 1; break;
        }
        nelems *= source_view.shape[i];
      }
      if (except) {
        break;
      }
      /* the buffers should be the itemsize */
      if (source_view.itemsize != sink_view.itemsize) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same itemsize, got source.itemsize == %llu and sink.itemsize == %llu", (unsigned long long)source_view.itemsize, (unsigned long long)sink_view.itemsize);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* buffers should be non-null */
      if (source_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "source buffer is NULL");
        except = 1; break;
      }
      if (sink_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "sink buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      if (sink_view.readonly) {
        PyErr_SetString(AtomiczError, "sink buffer is readonly");
        except = 1; break;
      }
      switch (sink_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only fetch_or itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      ret = do_fetch_or(source_view.buf, sink_view.buf, sink_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_source_view) {
			PyBuffer_Release(&source_view);
		}
		if (got_sink_view) {
			PyBuffer_Release(&sink_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return PyLong_FromLong(ret);
		}
	}

	static PyObject *
	fetch_xor_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long ret = 0;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer source_view;
		Py_buffer sink_view;
		int got_source_view = 0;
		int got_sink_view = 0;
		/* read the args */
		PyObject *obj_source;
		PyObject *obj_sink;
		do {
      if (!PyArg_ParseTuple(args, "OO", &obj_sink, &obj_source)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_source)) {
        PyErr_SetString(AtomiczError,
            "the source object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_sink)) {
        PyErr_SetString(AtomiczError,
            "the sink object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_source, &source_view, flags) < 0) {
        except = 1; break;
      } else {
        got_source_view = 1;
      }
      if (PyObject_GetBuffer(obj_sink, &sink_view, flags) < 0) {
        except = 1; break;
      } else {
        got_sink_view = 1;
      }
      /* the buffers should be the same rank */
      if (std::max(1,source_view.ndim) != std::max(1,sink_view.ndim)) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same rank, got source.ndim == %d and sink.ndim == %d", source_view.ndim, sink_view.ndim);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* the buffers should be the same shape */
      for (i = 0; i < source_view.ndim; i++) {
        if (source_view.shape[i] != sink_view.shape[i]) {
          PyErr_SetString(AtomiczError,
              "the source and sink buffers should be the same shape");
          except = 1; break;
        }
        nelems *= source_view.shape[i];
      }
      if (except) {
        break;
      }
      /* the buffers should be the itemsize */
      if (source_view.itemsize != sink_view.itemsize) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same itemsize, got source.itemsize == %llu and sink.itemsize == %llu", (unsigned long long)source_view.itemsize, (unsigned long long)sink_view.itemsize);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* buffers should be non-null */
      if (source_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "source buffer is NULL");
        except = 1; break;
      }
      if (sink_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "sink buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      if (sink_view.readonly) {
        PyErr_SetString(AtomiczError, "sink buffer is readonly");
        except = 1; break;
      }
      switch (sink_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only fetch_xor itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      ret = do_fetch_xor(source_view.buf, sink_view.buf, sink_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_source_view) {
			PyBuffer_Release(&source_view);
		}
		if (got_sink_view) {
			PyBuffer_Release(&sink_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return PyLong_FromLong(ret);
		}
	}

	static PyObject *
	fetch_and_python (PyObject *self, PyObject *args)
	{
    int i;
    unsigned long long ret = 0;
    unsigned long long nelems = 1;
		int except = 0;
		/* buffer views */
		Py_buffer source_view;
		Py_buffer sink_view;
		int got_source_view = 0;
		int got_sink_view = 0;
		/* read the args */
		PyObject *obj_source;
		PyObject *obj_sink;
		do {
      if (!PyArg_ParseTuple(args, "OO", &obj_sink, &obj_source)) {
        except = 1; break;
      }
      /* both objects should support the buffer interface */
      if (!PyObject_CheckBuffer(obj_source)) {
        PyErr_SetString(AtomiczError,
            "the source object does not support the buffer interface");
        except = 1; break;
      }
      if (!PyObject_CheckBuffer(obj_sink)) {
        PyErr_SetString(AtomiczError,
            "the sink object does not support the buffer interface");
        except = 1; break;
      }
      /* get the buffer view for each object */
      int flags = PyBUF_ND | PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE;
      if (PyObject_GetBuffer(obj_source, &source_view, flags) < 0) {
        except = 1; break;
      } else {
        got_source_view = 1;
      }
      if (PyObject_GetBuffer(obj_sink, &sink_view, flags) < 0) {
        except = 1; break;
      } else {
        got_sink_view = 1;
      }
      /* the buffers should be the same rank */
      if (std::max(1,source_view.ndim) != std::max(1,sink_view.ndim)) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same rank, got source.ndim == %d and sink.ndim == %d", source_view.ndim, sink_view.ndim);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* the buffers should be the same shape */
      for (i = 0; i < source_view.ndim; i++) {
        if (source_view.shape[i] != sink_view.shape[i]) {
          PyErr_SetString(AtomiczError,
              "the source and sink buffers should be the same shape");
          except = 1; break;
        }
        nelems *= source_view.shape[i];
      }
      if (except) {
        break;
      }
      /* the buffers should be the itemsize */
      if (source_view.itemsize != sink_view.itemsize) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "the source and sink buffers should be the same itemsize, got source.itemsize == %llu and sink.itemsize == %llu", (unsigned long long)source_view.itemsize, (unsigned long long)sink_view.itemsize);
        PyErr_SetString(AtomiczError, msg);
        except = 1; break;
      }
      /* buffers should be non-null */
      if (source_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "source buffer is NULL");
        except = 1; break;
      }
      if (sink_view.buf == NULL) {
        PyErr_SetString(AtomiczError, "sink buffer is NULL");
        except = 1; break;
      }
      /* buffers should point to exactly one item */
      if (nelems != 1) {
        PyErr_SetString(AtomiczError, "total element count must be 1, i.e. max(1,product(shape)) == 1");
        except = 1; break;
      }
      if (sink_view.readonly) {
        PyErr_SetString(AtomiczError, "sink buffer is readonly");
        except = 1; break;
      }
      switch (sink_view.itemsize)
      {
        case 1:
        case 2:
        case 4:
        case 8:
          break;
        default:
          PyErr_SetString(AtomiczError, "can only fetch_and itemsize of 1, 2, 4, or 8");
          except = 1;
          break;
      }
      if (except) break;
      ret = do_fetch_and(source_view.buf, sink_view.buf, sink_view.itemsize);
		} while(0);
//end:
		/* clean up the buffer views */
		if (got_source_view) {
			PyBuffer_Release(&source_view);
		}
		if (got_sink_view) {
			PyBuffer_Release(&sink_view);
		}
		/* return an appropriate value */
		if (except) {
			return NULL;
		} else {
      return PyLong_FromLong(ret);
		}
	}

	static PyMethodDef AtomiczModuleMethods[] = {
	 {"load", load_python, METH_VARARGS,
			"std::atomic::load from C++'s <atomic> library"},
	 {"store", store_python, METH_VARARGS,
			"std::atomic::store from C++'s <atomic> library"},
	 {"exchange", exchange_python, METH_VARARGS,
			"std::atomic::exchange from C++'s <atomic> library"},
	 {"compare_exchange", compare_exchange_python, METH_VARARGS,
			"std::atomic_compare_exchange_strong from C++'s <atomic> library"},
	 {"fetch_add", fetch_add_python, METH_VARARGS,
			"std::atomic::fetch_add from C++'s <atomic> library"},
	 {"fetch_sub", fetch_sub_python, METH_VARARGS,
			"std::atomic::fetch_sub from C++'s <atomic> library"},
	 {"fetch_or", fetch_or_python, METH_VARARGS,
			"std::atomic::fetch_or from C++'s <atomic> library"},
	 {"fetch_xor", fetch_xor_python, METH_VARARGS,
			"std::atomic::fetch_xor from C++'s <atomic> library"},
	 {"fetch_and", fetch_and_python, METH_VARARGS,
			"std::atomic::fetch_and from C++'s <atomic> library"},
		{NULL, NULL, 0, NULL}
	};

	static struct PyModuleDef AtomiczModule = {
		PyModuleDef_HEAD_INIT,
		"numpy_atomicz",
		NULL,
		-1,
		AtomiczModuleMethods,
		NULL,
		NULL,
		NULL,
		NULL
	};

	PyMODINIT_FUNC
	PyInit_atomicz(void)
	{
		PyObject *m;
		m = PyModule_Create(&AtomiczModule);
		if (m == NULL)
			return NULL;

		AtomiczError = PyErr_NewException("atomicz.error", NULL, NULL);
		Py_INCREF(AtomiczError);
		PyModule_AddObject(m, "error", AtomiczError);

#if 0
		RegisterNumpyAtomicz();
		Py_INCREF(&atomicz_type);
		Py_XINCREF(&NPyAtomicz_Descr);
		if (PyModule_AddObject(m, "atomicz", AtomiczDtype()) < 0)
		{
			Py_DECREF(&atomicz_type);
			Py_DECREF(m);
			return NULL;
		}
#endif

		return m;
	}
} // namespace greenwaves
