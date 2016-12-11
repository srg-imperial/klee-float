#!/usr/bin/env python
"""
  Take an unsigned bitvector and report as float/double.
  This is useful for taking SMT-LIBv2 float constant declarations
  e.g. ``((_ to_fp 11 53) (_ bv4617315517961601024 64))`` and
  working out what the approximate decimal value is.
"""
import argparse
import struct
import sys

def main(args):
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("type", type=str, choices=['f', 'd'])
  parser.add_argument("value", type=int)
  pargs = parser.parse_args(args)

  # Convert to binary data
  size = 0
  if pargs.type == 'f':
    size = 4
  elif pargs.type == 'd':
    size = 8
  else:
    print("Error unhandled type")
    return 1
  
  # Display in various forms
  hexStr = hex(pargs.value)[2:]
  # Pad out
  extraDigits = size*2 - len(hexStr)
  hexStr = ("0"* extraDigits) + hexStr
  print("As hex: 0x{}".format(hexStr))
  bitStr = bin(pargs.value)[2:]
  # Pad out
  extraDigits = size*8 - len(bitStr)
  bitStr = ("0"* extraDigits) + bitStr
  print("As bits: 0b{}".format(bitStr))
  print("{} bits".format(len(bitStr)))


  byteData = pargs.value.to_bytes(length=size, byteorder='little', signed=False)
  print("As bytes: {}".format(byteData))

  # Convert to float/double
  asFloat = struct.unpack_from('<{}'.format(pargs.type), byteData)[0]
  print("As float string: {}".format(asFloat))


  return 0;


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
