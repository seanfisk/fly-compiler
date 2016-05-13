#!/usr/bin/env python

from __future__ import print_function


def factorial(x):
    return x * factorial(x - 1) if x else 1


def print_factorial(x):
    print(factorial(x))


map(print_factorial, [14] * 4)
