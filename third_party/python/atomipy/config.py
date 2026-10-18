"""
Configuration settings for the atomipy package.

This module provides global settings that control the performance and behavior
of various atomipy functions. Users can modify these settings at runtime.
"""

# Threshold for switching between Direct O(N^2) and Sparse Cell List O(N) methods.
# Systems with fewer than this many atoms will use the Direct method by default.
SPARSE_THRESHOLD = 3000
