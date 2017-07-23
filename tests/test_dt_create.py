#!/usr/bin/env python3
# Copyright 2017 H2O.ai; Apache License Version 2.0;  -*- encoding: utf-8 -*-
#-------------------------------------------------------------------------------
#
# Test creating a datatable from various sources
#
#-------------------------------------------------------------------------------
import pytest
import datatable as dt


def test_create_from_list():
    d0 = dt.DataTable([1, 2, 3])
    assert d0.shape == (3, 1)
    assert d0.names == ("C1", )
    assert d0.types == ("int", )
    assert d0.internal.check()


def test_create_from_list_of_lists():
    d1 = dt.DataTable([[1, 2], [True, False], [.3, -0]], colnames="ABC")
    assert d1.shape == (2, 3)
    assert d1.names == ("A", "B", "C")
    assert d1.types == ("int", "bool", "real")
    assert d1.internal.check()


def test_create_from_tuple():
    d2 = dt.DataTable((3, 5, 6, 0))
    assert d2.shape == (4, 1)
    assert d2.types == ("int", )
    assert d2.internal.check()


def test_create_from_set():
    d3 = dt.DataTable({1, 13, 15, -16, -10, 7, 9, 1})
    assert d3.shape == (7, 1)
    assert d3.types == ("int", )
    assert d3.internal.check()


def test_create_from_nothing():
    d4 = dt.DataTable()
    assert d4.shape == (0, 0)
    assert d4.names == tuple()
    assert d4.types == tuple()
    assert d4.stypes == tuple()
    assert d4.internal.check()


def test_create_from_empty_list():
    d5 = dt.DataTable([])
    assert d5.shape == (0, 0)
    assert d5.names == tuple()
    assert d5.types == tuple()
    assert d5.stypes == tuple()
    assert d5.internal.check()


def test_create_from_empty_list_of_lists():
    d6 = dt.DataTable([[]])
    assert d6.shape == (0, 1)
    assert d6.names == ("C1", )
    assert d6.types == ("bool", )
    assert d6.internal.check()


def test_create_from_dict():
    d7 = dt.DataTable({"A": [1, 5, 10],
                       "B": [True, False, None],
                       "C": ["alpha", "beta", "gamma"]})
    assert d7.shape == (3, 3)
    assert d7.names == ("A", "B", "C")
    assert d7.types == ("int", "bool", "str")
    assert d7.internal.check()


def test_create_from_pandas(pandas):
    p = pandas.DataFrame({"A": [2, 5, 8], "B": ["e", "r", "qq"]})
    d = dt.DataTable(p)
    assert d.shape == (3, 2)
    assert set(d.names) == {"A", "B"}
    assert d.internal.check()


def test_create_from_pandas2(pandas, numpy):
    p = pandas.DataFrame(numpy.ones((3, 5)))
    d = dt.DataTable(p)
    assert d.shape == (3, 5)
    assert d.names == ("0", "1", "2", "3", "4")
    assert d.internal.check()


def test_create_from_pandas_series(pandas):
    p = pandas.Series([1, 5, 9, -12])
    d = dt.DataTable(p)
    assert d.shape == (4, 1)
    assert d.internal.check()
    assert d.topython() == [[1, 5, 9, -12]]


def test_bad():
    with pytest.raises(TypeError) as e:
        dt.DataTable("scratch")
    assert "Cannot create DataTable from 'scratch'" in str(e.value)


def test_issue_42():
    d = dt.DataTable([-1])
    assert d.shape == (1, 1)
    assert d.types == ("int", )
    assert d.internal.check()
    d = dt.DataTable([-1, 2, 5, "hooray"])
    assert d.shape == (4, 1)
    assert d.types == ("str", )
    assert d.internal.check()
