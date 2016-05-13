#!/usr/bin/env/python

from __future__ import print_function


def recursive_add(x):
    return x + recursive_add(x - 1) if x else 0


def print_recursive_add(x):
    print(recursive_add(x))


map(print_recursive_add, [9999] * 4)
