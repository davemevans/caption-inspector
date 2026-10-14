#!/usr/bin/env python
# -*- coding: latin-1 -*-

import shutil

import pytest


@pytest.fixture(autouse=True)
def clean_tmp_dir():
    # Tests write their output under ./tmp and remove it when they pass. A failing test
    # leaves it behind, so clear it first: the next test then starts from an empty
    # directory instead of failing on the leftovers.
    shutil.rmtree('./tmp', ignore_errors=True)
    yield
