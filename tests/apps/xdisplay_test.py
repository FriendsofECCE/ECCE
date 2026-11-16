#!/usr/bin/env python3
"""Test X displays must leave desktop display numbers free (#1024)."""
import os
import unittest
from unittest import mock

import xdisplay


class DisplayChoiceTest(unittest.TestCase):
    def setUp(self):
        self.env = mock.patch.dict(os.environ, ECCE_TEST_XDISPLAYS="")
        self.env.start()
        self.addCleanup(self.env.stop)

    def test_default_starts_above_desktop_range(self):
        with mock.patch.object(os.path, "exists", return_value=False):
            self.assertEqual(xdisplay._freeDisplay(), 90)

    def test_skips_both_socket_and_lock(self):
        occupied = {"/tmp/.X11-unix/X90", "/tmp/.X91-lock"}
        with mock.patch.object(os.path, "exists", side_effect=occupied.__contains__):
            self.assertEqual(xdisplay._freeDisplay(), 92)

    def test_low_explicit_displays_never_launch(self):
        with mock.patch.object(xdisplay, "findXvfb", return_value="Xvfb"), \
                mock.patch.object(xdisplay.subprocess, "Popen") as launch:
            for number in range(10):
                with self.subTest(number=number), self.assertRaises(xdisplay.DisplayUnavailable):
                    xdisplay.Display(number=number).__enter__()
            launch.assert_not_called()

    def test_unsafe_ranges_never_launch(self):
        with mock.patch.object(xdisplay, "findXvfb", return_value="Xvfb"), \
                mock.patch.object(xdisplay.subprocess, "Popen") as launch:
            for span in ("0-9", "0-99", "9-90", "99-90"):
                with self.subTest(span=span), \
                        mock.patch.dict(os.environ, ECCE_TEST_XDISPLAYS=span), \
                        self.assertRaises(xdisplay.DisplayUnavailable):
                    xdisplay.Display().__enter__()
            launch.assert_not_called()

    def test_range_is_respected_and_exhaustion_fails(self):
        with mock.patch.dict(os.environ, ECCE_TEST_XDISPLAYS="160-169"):
            with mock.patch.object(os.path, "exists", return_value=False):
                self.assertEqual(xdisplay._freeDisplay(), 160)
            with mock.patch.object(os.path, "exists", return_value=True):
                with self.assertRaises(xdisplay.DisplayUnavailable):
                    xdisplay._freeDisplay()

    def test_real_servers_keep_desktop_numbers_free(self):
        try:
            xdisplay.findXvfb()
        except xdisplay.DisplayUnavailable as exc:
            self.skipTest(str(exc))
        with xdisplay.Display() as first, xdisplay.Display() as second:
            self.assertGreaterEqual(first.number, 90)
            self.assertGreaterEqual(second.number, 90)
            self.assertNotEqual(first.number, second.number)

    def test_lost_race_is_not_shared(self):
        # xdpyinfo answers from the winner's server; only -displayfd tells.
        try:
            xdisplay.findXvfb()
        except xdisplay.DisplayUnavailable as exc:
            self.skipTest(str(exc))
        with xdisplay.Display() as first:
            with self.assertRaises(xdisplay.DisplayUnavailable):
                xdisplay.Display(number=first.number).__enter__()
            free = xdisplay._freeDisplay()
            with mock.patch.object(xdisplay, "_freeDisplay",
                                   side_effect=[first.number, free]):
                with xdisplay.Display() as second:
                    self.assertEqual(second.number, free)


if __name__ == "__main__":
    unittest.main()
