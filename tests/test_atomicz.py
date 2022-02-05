import unittest

import atomicz
import numpy as np
import ctypes as c
i64 = c.c_longlong

class AtomiczTestCase(unittest.TestCase):
  def test_basic(self):
    self.assertEqual(1, 1)

  def test_cas(self):
    obj = i64(0)
    expected = i64(0)
    desired = i64(123)
    self.assertTrue(atomicz.compare_exchange(obj, expected, desired))
    self.assertEqual(obj.value, desired.value)
    desired = i64(42)
    self.assertFalse(atomicz.compare_exchange(obj, expected, desired))
    self.assertNotEqual(obj.value, desired.value)
    expected = i64(obj.value)
    self.assertTrue(atomicz.compare_exchange(obj, expected, desired))
    self.assertEqual(obj.value, desired.value)

if __name__ == '__main__':
  unittest.main()
